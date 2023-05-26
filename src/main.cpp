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
    olc::Pixel colour;
    const int lane;
    int depth;
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
    std::vector<Ball> balls;

    bool OnUserCreate() override
    {
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(olc::BLACK);

        update();
        draw();

        return true;
    }

    void update(){
        if(GetKey(olc::LEFT).bPressed){
            player_lane = std::max(0, player_lane-1);
        }
        if(GetKey(olc::RIGHT).bPressed){
            player_lane = std::min(LANES-1, player_lane+1);
        }
    }

    void draw(){
        draw_lanes();
        draw_player();
    }

    void draw_lanes() {
        for (int l = 0; l < LANES; l++){
            DrawRect({l*LANE_WIDTH, 0}, {LANE_WIDTH, LANE_DEPTH});
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

