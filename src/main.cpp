#include "olcPixelGameEngine.h"
#include "olcSoundWaveEngine.h"
#include "AssetManager.h"
#include "math.h"

using am = AssetManager;

const int LANES = 5;
const int LANE_WIDTH = 50;
const int LANE_DEPTH = 400;
const int PLAYER_WIDTH = 40;
const int PLAYER_DEPTH = 20;

struct Ball {
    enum e_state {
        FALLING,
        STOPPED,
        FADING,
        HELD,
        SCORING
    };

    e_state state = FALLING;
    olc::Pixel colour;
    float depth;
    int rad = LANE_WIDTH/2 -2;
    int lane;
    const int speed = 50;
    const int fade_rate = 10;
    Ball* container = nullptr;
    Ball* contains = nullptr;

    Ball(olc::Pixel colour, int lane) : colour(colour), lane(lane), depth(-20.0f){
        colour.a = 0xFF;
    }

    ~Ball() {
        if (contains != nullptr)
            delete contains;
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
                default: break;
            }

            //State action
            switch(state){
                case FALLING:
                    depth += speed * fElapsedTime;
                    break;
                case FADING:
                    colour.a -= fade_rate * fElapsedTime;
                    break;
                default: break;
            }
        }

        if (contains != nullptr){
            contains->update(fElapsedTime, lanes_running, player_pos);
        }
    }

    void insert(Ball* to_insert){
        to_insert->rad = rad - 2;
        to_insert->container = this;
        contains = to_insert;
    }

    void draw(olc::PixelGameEngine &pge){
        
        switch(state){
            case FALLING:
            case STOPPED:
                pge.DrawCircle({lane*LANE_WIDTH + LANE_WIDTH/2, depth}, rad, colour);
                break;
            case FADING:
                pge.SetPixelMode(olc::Pixel::ALPHA);
                pge.DrawCircle({lane*LANE_WIDTH + LANE_WIDTH/2, depth}, rad, colour);
                pge.SetPixelMode(olc::Pixel::NORMAL);
                break;
            default: break;

        }

        if (contains != nullptr){
            contains->draw(pge);
        }
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

    bool OnUserCreate() override
    {
        for (int i=0; i < LANES; i++){
            lane_running.push_back(true);
        }

        balls.push_back(new Ball(olc::BLUE, 0));
        balls.push_back(new Ball(olc::RED, 4));
        balls.push_back(new Ball(olc::GREEN, 2));
        balls[0]->insert(new Ball(olc::YELLOW, 1));
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(olc::BLACK);

        update(fElapsedTime);
        draw();

        return true;
    }

    void update(float fElapsedTime){
        if(GetKey(olc::LEFT).bPressed){
            player_lane = std::max(0, player_lane-1);
        }
        if(GetKey(olc::RIGHT).bPressed){
            player_lane = std::min(LANES-1, player_lane+1);
        }
        if(GetKey(olc::SPACE).bPressed){
            lane_running[player_lane] = !lane_running[player_lane];
        }

        std::vector<int> to_rm;
        int i = 0;
        for (auto &ball : balls){
            uint8_t old_a = ball->colour.a;
            ball->update(fElapsedTime, lane_running, player_lane);
            if (ball->colour.a > old_a){
                to_rm.push_back(i);
            }
            i++;
        }
        for (const auto &i : to_rm){
            delete balls[i];
            balls.erase(balls.begin()+i);
        }
    }

    void draw(){
        draw_balls();
        draw_lanes();
        draw_player();
    }

    void draw_lanes() {
        for (int l = 0; l < LANES; l++){
            DrawRect({l*LANE_WIDTH, 0}, {LANE_WIDTH, LANE_DEPTH});
        }
        FillRect(
            {0,LANE_DEPTH+1},
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
        DrawRect(
            {(player_lane*LANE_WIDTH) + (LANE_WIDTH-PLAYER_WIDTH)/2, LANE_DEPTH + 4},
            {PLAYER_WIDTH, PLAYER_DEPTH}
        );
    }
};


int main()
{
    MJ113 game;
    if(game.Construct(LANE_WIDTH*LANES+1, LANE_DEPTH + 4 + 4 + PLAYER_DEPTH, 2, 2))
        game.Start();

    return 0;
}

