#include "olcPixelGameEngine.h"
#include "olcSoundWaveEngine.h"
#include "AssetManager.h"
#include "math.h"
#include <random>

using am = AssetManager;

const int PREVIEW_DEPTH = 20;
const int LANES = 5;
const int LANE_START = PREVIEW_DEPTH + 1;
const int LANE_WIDTH = 50;
const int LANE_DEPTH = 400;
const int PLAYER_WIDTH = 40;
const int PLAYER_CORNER = 4;
const int COLOUR_COUNT = 4;
const int STARTER_WIDTH = LANE_WIDTH/2 -2;
const int ACCEPTOR_DEPTH = LANE_WIDTH;
const olc::Pixel COLOURS[] = {
    olc::GREEN,
    olc::RED,
    olc::BLUE,
    olc::YELLOW
};

struct Ball {
    enum e_state {
        FALLING,
        STOPPED,
        TO_REMOVE,
        FADING,
        HELD,
        SCORING,
        TARGET
    };

    e_state state = FALLING;
    olc::Pixel colour;
    float depth;
    int rad = STARTER_WIDTH;
    int lane;
    const int speed = 30;
    const int fade_rate = 1;
    Ball* container = nullptr;
    Ball* contains = nullptr;

    Ball(olc::Pixel colour, int lane) : colour(colour), lane(lane), depth(-20.0f){
        colour.a = 0xFF;
    }

    ~Ball() {
        if (contains != nullptr)
            delete contains;
    }

    int get_count(){
        int c = 1;
        if (contains != nullptr){
            c += contains->get_count();
        }
        return c;
    }

    bool operator==(const Ball& other){
        if (colour != other.colour) return false;
        if (contains == nullptr && other.contains == nullptr) return true;
        if (contains == nullptr || other.contains == nullptr) return false;
        return *contains == *(other.contains);
    }

    void update(float fElapsedTime, std::vector<bool>& lanes_running, int player_pos) {
        //State update
        if (container != nullptr){
            state = container->state;
            depth = container->depth;
            lane = container->lane;
            colour.a = container->colour.a;
        } else {
            switch(state){
                case FALLING:
                    if (depth + rad >= LANE_DEPTH){
                        state = FADING;
                    } else
                    if (lanes_running[lane] == false){
                        state = STOPPED;
                    }
                    break;
                case STOPPED:
                    if (lanes_running[lane]){
                        state = FALLING;
                    }
                    break;
                case HELD:
                    lane = player_pos;
                    break;
                default: break;
            }

            //State action
            switch(state){
                case FALLING:
                    depth += speed * fElapsedTime;
                    break;
                case FADING: {
                    colour.a -= fade_rate * fElapsedTime;
                    if (colour.a == 0){
                        state = TO_REMOVE;
                    }
                    break;
                }
                default: break;
            }
        }

        if (contains != nullptr){
            contains->update(fElapsedTime, lanes_running, player_pos);
        }
    }

    void _insert(){
        rad = container->rad - 2;
        if (contains != nullptr){
            contains->_insert();
        }
    }

    void insert(Ball* to_insert){
        if (contains == nullptr){
            to_insert->container = this;
            contains = to_insert;
            contains->_insert();
        } else {
            contains->insert(to_insert);
        }
    }

    void make_held(){
        state = HELD;
    }

    void draw(olc::PixelGameEngine &pge){
        
        switch(state){
            case FALLING:
            case STOPPED:
            case TARGET:
                pge.DrawCircle({lane*LANE_WIDTH + LANE_WIDTH/2, LANE_START + depth}, rad, colour);
                break;
            case FADING:
                pge.SetPixelMode(olc::Pixel::ALPHA);
                pge.DrawCircle({lane*LANE_WIDTH + LANE_WIDTH/2, LANE_START +  depth}, rad, colour);
                pge.SetPixelMode(olc::Pixel::NORMAL);
                break;
            case HELD:
                pge.DrawCircle(
                    {lane*LANE_WIDTH + LANE_WIDTH/2,
                    LANE_START + LANE_DEPTH + 4 + PLAYER_WIDTH/2},
                    rad, colour
                );
                break;

            default: break;

        }

        if (contains != nullptr){
            contains->draw(pge);
        }
    }


};

struct BallGenerator {
    float min_time = 3.0f;
    float max_time = 5.0f;
    std::random_device dev;
    std::mt19937 rng;

    BallGenerator() :
        rng(std::mt19937(dev())) 
    {

    }

    Ball* get_target() {
        int depth = 4;
        Ball* broot = new Ball(COLOURS[rand() % COLOUR_COUNT], -1);
        broot->state = Ball::TARGET;
        broot->lane = 2;
        broot->depth = LANE_START + LANE_DEPTH + LANE_WIDTH + 2;
        Ball* bcurrent = broot;

        for (int i = 1; i < depth; i++){
            Ball* bnew = new Ball(COLOURS[rand() % COLOUR_COUNT], -1);
            bcurrent->insert(bnew);
            bcurrent = bnew;
        }

        return broot;
    }

    std::pair<int,olc::Pixel> get_next() { 
        std::uniform_int_distribution<std::mt19937::result_type> dist(min_time,max_time);
        olc::Pixel colour = COLOURS[rand() % COLOUR_COUNT];

        return {dist(rng), colour};
    }
    
};


class MJ113 : public olc::PixelGameEngine
{
public:
    MJ113()
    {
        sAppName = "Dogeballs?";
        player_lane = ceil(LANES/2);
    }

private:
    int player_lane;
    std::vector<Ball*> balls;
    std::vector<bool> lane_running;
    std::vector<std::tuple<float,float,olc::Pixel>> lane_timer;
    bool reaching = false;    
    Ball* held = nullptr;
    int max_time = 5;
    BallGenerator generator;
    Ball* target = nullptr;


    bool OnUserCreate() override
    {
        for (int i=0; i < LANES; i++){
            lane_running.push_back(true);
            auto [starter_time, colour] = generator.get_next();
            lane_timer.push_back({starter_time, starter_time, colour});
        }

        target = generator.get_target();

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(olc::BLACK);

        update(fElapsedTime);
        draw();

        return true;
    }

    std::pair<int,Ball*> get_closest_ball(){
        int closest_index = -1;
        float max_depth = 0.0f;
        for (size_t i = 0; i < balls.size(); i++){
            auto b = balls[i];
            if (b->lane != player_lane) continue;
            if (b->depth < max_depth) continue;
            closest_index = i;
            max_depth = b->depth;
        }

        return {closest_index, closest_index == -1 ? nullptr : balls[closest_index]};
    }

    void update(float fElapsedTime){
        if (reaching){
            if (GetKey(olc::UP).bReleased){
                reaching = false;
                lane_running[player_lane] = true;
                auto [ind, ball] = get_closest_ball();

                if (ind != -1){
                    if (held != nullptr){
                        ball->insert(held);
                        held = nullptr;
                    } else {
                        ball->make_held();
                        held = ball;
                        balls.erase(balls.begin()+ind);
                    }
                }
            }
        } else {
            if(GetKey(olc::LEFT).bPressed){
                player_lane = std::max(0, player_lane-1);
            }
            if(GetKey(olc::RIGHT).bPressed){
                player_lane = std::min(LANES-1, player_lane+1);
            }
            if(GetKey(olc::SPACE).bPressed){
                if (lane_running[player_lane]){
                    for (int i=0; i < lane_running.size(); i++){
                        lane_running[i] = true;
                    }
                    lane_running[player_lane] = false;
                } else {
                    lane_running[player_lane] = true;
                }
            }
            if (GetKey(olc::UP).bPressed){
                reaching = true;
                lane_running[player_lane] = false;
            }
            if (GetKey(olc::DOWN).bPressed && held != nullptr){
                if (*held == *target){
                    std::cout << "match!" << std::endl;
                    delete held;
                    held = nullptr;
                    delete target;
                    target = generator.get_target();
                } else {
                    std::cout << "mismatch" << std::endl;
                }
            }
        }

        for (auto &ball : balls){
            ball->update(fElapsedTime, lane_running, player_lane);
        }

        if (held!=nullptr){
            held->update(fElapsedTime, lane_running, player_lane);
        }

        if (target!=nullptr){
            target->update(fElapsedTime, lane_running, player_lane);
        }

        balls.erase(std::remove_if(
            balls.begin(), balls.end(),
            [](const Ball* b) { 
                if (b->state == Ball::TO_REMOVE){
                    delete b;
                    return true;
                }
                return false;
            }),
            balls.end()
        );

        for (int i=0; i < lane_timer.size(); i++){
            auto &[current_time, lane_max_time, colour] = lane_timer[i];
            if (lane_running[i]){
                current_time -= fElapsedTime;
            }

            if (current_time < 0.0f){
                balls.push_back(new Ball(colour, i));
                auto [new_time, new_colour] = generator.get_next();
                lane_timer[i] = {new_time, new_time, new_colour};
            }
        }
    }

    void draw(){
        draw_balls();
        draw_lanes();
        draw_player();
        draw_timer();
        draw_acceptor();
    }

    void draw_acceptor() {
        target->draw(*this);
    }

    void draw_timer() {
        FillRect(
            {0, 0},
            {ScreenWidth(), PREVIEW_DEPTH},
            olc::BLACK
        );
        for (int l = 0; l < LANES; l++){
            auto &[current_time, lane_max_time, colour] = lane_timer[l];
            DrawRect({l*LANE_WIDTH, 0}, {LANE_WIDTH, PREVIEW_DEPTH});
            FillRect({l*LANE_WIDTH+1, 0+1}, {LANE_WIDTH*(current_time/lane_max_time), PREVIEW_DEPTH-1}, colour);
        }
    }

    void draw_lanes() {
        for (int l = 0; l < LANES; l++){
            DrawRect({l*LANE_WIDTH, LANE_START}, {LANE_WIDTH, LANE_DEPTH});
        }
        FillRect(
            {0, LANE_START + LANE_DEPTH+1},
            {ScreenWidth(), ScreenHeight()-LANE_DEPTH},
            olc::BLACK
        );
    }

    void draw_balls() {
        for (const auto &ball : balls){
            ball->draw(*this);
        }
    }

    void draw_player() {
        olc::vi2d player_top_left = {(player_lane*LANE_WIDTH) + (LANE_WIDTH-PLAYER_WIDTH)/2, LANE_START + LANE_DEPTH + 6};
        olc::vi2d player_size = {PLAYER_WIDTH, PLAYER_WIDTH};

        DrawRect(player_top_left, player_size);
        FillRect(
            player_top_left + olc::vi2d(PLAYER_CORNER, 0),
            player_size - olc::vi2d(2*PLAYER_CORNER, -1),
            olc::BLACK
        );
        FillRect(
            player_top_left + olc::vi2d(0, PLAYER_CORNER),
            player_size - olc::vi2d(-1, 2*PLAYER_CORNER),
            olc::BLACK
        );
        
        if (held != nullptr){
            held->draw(*this);
        }
    }
};


int main()
{
    srand(time(NULL));
    MJ113 game;
    if(game.Construct(
        LANE_WIDTH*LANES+1,
        PREVIEW_DEPTH + LANE_DEPTH + 4 + 4 + PLAYER_WIDTH + ACCEPTOR_DEPTH,
        2, 2)
    ) game.Start();

    return 0;
}

