// libraries
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>
#include <array>

// sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

// headers
#include "../common/func.cpp"
#include "../common/fonts.h"
#include "../common/images.h"
#include "colors.h"

void DrawTiles(SDL_Renderer *rendere, int width, int height, int tileSize, int offset, std::vector<int> minePos, Text numbers, std::vector<int> field, std::vector<int> tileState, Image flag);
std::vector<int> PopulateMines(int width, int height, std::mt19937 rand);
std::vector<int> CreateField(int width, int height, std::vector<int> minePos, std::vector<int> edges);
std::vector<int> DefineEdges(int width, int height);
std::vector<std::array<int, 2>> TilePos(int width, int height, int tileSize, int offset);
std::vector<int> TileClicked(int mouseX, int mouseY, std::vector<int> tileState, std::vector<std::array<int, 2>> tilePos, int tileSize, std::vector<int> field, std::vector<int> edges, int width, int height, std::vector<int> minePos);
std::vector<int> RevealEmpty(int width, int height, int tileClickedId, std::vector<int> field, std::vector<int> edges, std::vector<int> tileState);
int ClickPos(int mouseX, int mouseY, std::vector<std::array<int, 2>> tilePos, int tileSize);
std::vector<int> PlaceFlag(int mouseX, int mouseY, std::vector<int> tileState, std::vector<std::array<int, 2>> tilePos, int tileSize, std::vector<int> minePos);

int main(int argc, char *argv[])
{
    int mouseX { };
    int mouseY { };

    std::mt19937 rand { std::random_device{}() };
    std::cout << "enter width: ";
    int width { };
    std::cin >> width;

    std::cout << "enter height: ";
    int height { };
    std::cin >> height;

    int tileSize { 50 };
    int offset { 5 };

    int windowWidth { width * (tileSize + offset) + offset};
    int windowHeight { height * (tileSize + offset) + offset};

    std::vector<int> edges { DefineEdges(width, height) };
    std::vector<int> minePos { PopulateMines(width, height, rand)};
    std::vector<int> field { CreateField(width, height, minePos, edges) };
    // 0 = hidden, 1 = revealed, 2 = flagged
    std::vector<int> tileState(field.size());
    // [0] = x, [1] = y
    std::vector<std::array<int, 2>> tilePos { TilePos(width, height, tileSize, offset) };


    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    int imgFlags { IMG_INIT_PNG | IMG_INIT_JPG };
    if(!(IMG_Init(imgFlags) & imgFlags))
        std::cout << "error: " << IMG_GetError() << '\n';

    SDL_Window *window = SDL_CreateWindow("minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer *rendere = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SetRenderDrawBlendMode(rendere, SDL_BLENDMODE_BLEND);

    Text numbers;
    numbers.color = { 0, 0, 0, 255 };
    numbers.fontSize = 20;
    numbers.init(rendere, "arial.ttf");

    Image flag;
    flag.init(rendere, "flag.png");
    flag.resizeKA('w', tileSize, windowHeight);

    Text bannerText;
    bannerText.color = { 0, 0, 0, 255 };
    bannerText.fontSize = 100;
    bannerText.init(rendere, "arial.ttf");

    SDL_Event ev;
    bool running { true };
    bool won { false };
    bool lost { false };

    while(running)
    {
        SDL_GetMouseState(&mouseX, &mouseY);
        while(SDL_PollEvent(&ev) > 0)
        {
            if(ev.type == SDL_QUIT)
                running = false;
            else if(ev.type == SDL_KEYDOWN)
            {
                switch(ev.key.keysym.sym)
                {
                    case SDLK_q:
                        running = false;
                        break;
                }
            } else if(ev.type == SDL_MOUSEBUTTONDOWN)
            {
                switch(ev.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        tileState = TileClicked(mouseX, mouseY, tileState, tilePos, tileSize, field, edges, width, height, minePos);
                        break;
                    case SDL_BUTTON_RIGHT:
                        tileState = PlaceFlag(mouseX, mouseY, tileState, tilePos, tileSize, minePos);
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(rendere, 255, 255, 255, 255);

        SDL_RenderClear(rendere);

        DrawTiles(rendere, width, height, tileSize, offset, minePos, numbers, field, tileState, flag);

        if(won || lost)
        {
            SDL_Rect overlay;
            overlay.w = windowWidth;
            overlay.h = windowHeight;
            overlay.x = overlay.y = 0;

            SDL_SetRenderDrawColor(rendere, 255, 255, 255, 150);
            SDL_RenderFillRect(rendere, &overlay);
            SDL_SetRenderDrawColor(rendere, 255, 255, 255, 255);

            bannerText.x = windowWidth / 2 - bannerText.w / 2;
            bannerText.y = windowHeight / 2 - bannerText.h / 2;
        }

        if(won)
        {
            bannerText.textString = "You won!";
            bannerText.draw(rendere);
        } else if(lost)
        {
            bannerText.textString = "You lost!";
            bannerText.draw(rendere);
        }

        SDL_RenderPresent(rendere);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(rendere);

    window = nullptr;
    rendere = nullptr;

    SDL_Quit();
    TTF_Quit();
    
    return 0;
}

void DrawTiles(SDL_Renderer *rendere, int width, int height, int tileSize, int offset, std::vector<int> minePos, Text numbers, std::vector<int> field, std::vector<int> tileState, Image flag)
{
    SDL_Rect tileRect;
    tileRect.w = tileRect.h = tileSize;

    tileRect.x = tileRect.y = offset;

    int count { };

    SDL_SetRenderDrawColor(rendere, 0, 0, 0, 255);
    for(int y { }; y<height; ++y)
    {
        Color drawColor;
        for(int x { }; x<width; ++x)
        {
            bool drawNumber { false };
            if(std::find(minePos.begin(), minePos.end(), count) == minePos.end())
            {

                numbers.textString = std::to_string(field[count]);
                numbers.x = (tileRect.x + tileRect.w / 2) - (numbers.w / 2);
                numbers.y = (tileRect.y + tileRect.h / 2) - (numbers.h / 2);
                drawNumber = true;
            }

            if(tileState[count] == 1)
            {
                switch(field[count])
                {
                    case 0:
                        drawColor = color::zero;
                        break;
                    case 1:
                        drawColor = color::one;
                        break;
                    case 2:
                        drawColor = color::two;
                        break;
                    case 3:
                        drawColor = color::three;
                        break;
                    case 4:
                        drawColor = color::four;
                        break;
                    case 5:
                        drawColor = color::five;
                        break;
                    case 6:
                        drawColor = color::six;
                        break;
                    case 7:
                        drawColor = color::seven;
                        break;
                    case 8:
                        drawColor = color::eight;
                        break;
                    default:
                        drawColor = color::unopened;
                        break;
                }
            } else
                drawColor = color::unopened;
            
            SDL_SetRenderDrawColor(rendere, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

            SDL_RenderFillRect(rendere, &tileRect);
            if(drawNumber && tileState[count] == 1)
                numbers.draw(rendere);
            else if(tileState[count] == 2)
            {
                flag.x = tileRect.x;
                flag.y = tileRect.y;
                flag.draw(rendere);
            }
            tileRect.x += tileSize + offset;
            ++count;
        }

        tileRect.x = offset;
        tileRect.y += tileSize + offset;
    }
}

std::vector<int> PopulateMines(int width, int height, std::mt19937 rand)
{
    std::cout << "enter mine-count: ";
    int mineCount { };
    std::cin >> mineCount;

    std::vector<int> field(width*height);
    std::iota(std::begin(field), std::end(field), 0);
    std::shuffle(std::begin(field), std::end(field), rand);

    field.resize(field.size()-(field.size() - mineCount));

    return field;
}

std::vector<int> CreateField(int width, int height, std::vector<int> minePos, std::vector<int> edges)
{
    std::vector<int> field(height * width);
    std::vector<int> opEdges;
    for(int i : edges)
        opEdges.push_back(i + (width - 1));


    for(int i { 0 }; i<field.size(); ++i)
    {
        int mineCount { };
        if(std::find(edges.begin(), edges.end(), i) == edges.end() && std::find(opEdges.begin(), opEdges.end(), i) == opEdges.end())
        {
            if(std::find(minePos.begin(), minePos.end(), i - width - 1) != minePos.end() && i - width - 1 > -1 && i - width - 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i - width) != minePos.end() && i - width > -1 && i - width < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i - width + 1) != minePos.end() && i - width + 1 > -1 && i - width + 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i - 1) != minePos.end() && i - 1 > -1 && i - 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + 1) != minePos.end() && i + 1 > -1 && i + 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width - 1) != minePos.end() && i + width - 1 > -1 && i + width - 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width) != minePos.end() && i + width > -1 && i + width < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width + 1) != minePos.end() && i + width + 1 > -1 && i + width + 1< field.size())
                ++mineCount;
        } else if(std::find(opEdges.begin(), opEdges.end(), i) == opEdges.end())
        {
            if(std::find(minePos.begin(), minePos.end(), i - width) != minePos.end() && i - width > -1 && i - width < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i - width + 1) != minePos.end() && i - width + 1 > -1 && i - width + 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + 1) != minePos.end() && i + 1 > -1 && i + 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width) != minePos.end() && i + width > -1 && i + width < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width + 1) != minePos.end() && i + width + 1 > -1 && i + width + 1< field.size())
                ++mineCount;
        } else
        {
            if(std::find(minePos.begin(), minePos.end(), i - width - 1) != minePos.end() && i - width - 1 > -1 && i - width - 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i - width) != minePos.end() && i - width > -1 && i - width < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i - 1) != minePos.end() && i - 1 > -1 && i - 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width - 1) != minePos.end() && i + width - 1 > -1 && i + width - 1 < field.size())
                ++mineCount;
            if(std::find(minePos.begin(), minePos.end(), i + width) != minePos.end() && i + width > -1 && i + width < field.size())
                ++mineCount;
        }

        field[i] = mineCount;
    }

    return field;
}

std::vector<int> DefineEdges(int width, int height)
{
    std::vector<int> edges;
    int count { 0 };
    for(int i { }; i<height; ++i)
    {
        edges.push_back(count);
        count += width;
    }

    return edges;
}

std::vector<std::array<int, 2>> TilePos(int width, int height, int tileSize, int offset)
{
    std::vector<std::array<int, 2>> result;
    std::array<int, 2> count { offset, offset };
    for(int i { }; i<width * height; ++i)
    {
        result.push_back(count);
        if((i + 1) % width  == 0)
        {
            count[0] = offset;
            count[1] += tileSize + offset;
            continue;
        }
        count[0] += tileSize + offset;
    }

    return result;
}

std::vector<int> TileClicked(int mouseX, int mouseY, std::vector<int> tileState, std::vector<std::array<int, 2>> tilePos, int tileSize, std::vector<int> field, std::vector<int> edges, int width, int height, std::vector<int> minePos)
{
    int tileClickedId { ClickPos(mouseX, mouseY, tilePos, tileSize) };

    if(tileClickedId > -1 && tileClickedId < tileState.size() && field[tileClickedId] == 0 && std::find(minePos.begin(), minePos.end(), tileClickedId) == minePos.end() && tileState[tileClickedId] == 0)
    {
        tileState = RevealEmpty(width, height, tileClickedId, field, edges, tileState);
        tileState[tileClickedId] = 1;
    } else if(tileClickedId > -1 && tileClickedId < tileState.size() && std::find(minePos.begin(), minePos.end(), tileClickedId) == minePos.end() && tileState[tileClickedId] == 0)
        tileState[tileClickedId] = 1;

    return tileState;
}

std::vector<int> RevealEmpty(int width, int height, int tileClickedId, std::vector<int> field, std::vector<int> edges, std::vector<int> tileState)
{
    std::vector<int> opEdges;
    for(int i : edges)
        opEdges.push_back(i + (width - 1));

    if(std::find(edges.begin(), edges.end(), tileClickedId) == edges.end() && std::find(opEdges.begin(), opEdges.end(), tileClickedId) == opEdges.end())
    {
        if(tileClickedId - width - 1 > -1 && tileClickedId - width - 1 < field.size())
            tileState[tileClickedId - width - 1] = 1;
        if(tileClickedId - width > -1 && tileClickedId - width < field.size())
            tileState[tileClickedId - width] = 1;
        if(tileClickedId - width + 1 > -1 && tileClickedId - width + 1 < field.size())
            tileState[tileClickedId - width + 1] = 1;
        if(tileClickedId - 1 > -1 && tileClickedId - 1 < field.size())
            tileState[tileClickedId - 1] = 1;
        if(tileClickedId + 1 > -1 && tileClickedId + 1 < field.size())
            tileState[tileClickedId + 1] = 1;
        if(tileClickedId + width - 1 > -1 && tileClickedId + width - 1 < field.size())
            tileState[tileClickedId + width - 1] = 1;
        if(tileClickedId + width > -1 && tileClickedId + width < field.size())
            tileState[tileClickedId + width] = 1;
        if(tileClickedId + width + 1 > -1 && tileClickedId + width + 1< field.size())
            tileState[tileClickedId + width + 1] = 1;
    } else if(std::find(opEdges.begin(), opEdges.end(), tileClickedId) == opEdges.end())
    {
        if(tileClickedId - width > -1 && tileClickedId - width < field.size())
            tileState[tileClickedId - width] = 1;
        if(tileClickedId - width + 1 > -1 && tileClickedId - width + 1 < field.size())
            tileState[tileClickedId - width + 1] = 1;
        if(tileClickedId + 1 > -1 && tileClickedId + 1 < field.size())
            tileState[tileClickedId + 1] = 1;
        if(tileClickedId + width > -1 && tileClickedId + width < field.size())
            tileState[tileClickedId + width] = 1;
        if(tileClickedId + width + 1 > -1 && tileClickedId + width + 1< field.size())
            tileState[tileClickedId + width + 1] = 1;
    } else
    {
        if(tileClickedId - width - 1 > -1 && tileClickedId - width - 1 < field.size())
            tileState[tileClickedId - width - 1] = 1;
        if(tileClickedId - width > -1 && tileClickedId - width < field.size())
            tileState[tileClickedId - width] = 1;
        if(tileClickedId - 1 > -1 && tileClickedId - 1 < field.size())
            tileState[tileClickedId - 1] = 1;
        if(tileClickedId + width - 1 > -1 && tileClickedId + width - 1 < field.size())
            tileState[tileClickedId + width - 1] = 1;
        if(tileClickedId + width > -1 && tileClickedId + width < field.size())
            tileState[tileClickedId + width] = 1;
    }

    return tileState;
}

int ClickPos(int mouseX, int mouseY, std::vector<std::array<int, 2>> tilePos, int tileSize)
{
    int tileClickedId;

    for(int i { }; i<tilePos.size(); ++i)
    {
        if(mouseX > tilePos[i][0] && mouseX < tilePos[i][0] + tileSize && mouseY > tilePos[i][1] && mouseY < tilePos[i][1] + tileSize)
        {
            tileClickedId = i;
            break;
        }
    }

    return tileClickedId;
}
std::vector<int> PlaceFlag(int mouseX, int mouseY, std::vector<int> tileState, std::vector<std::array<int, 2>> tilePos, int tileSize, std::vector<int> minePos)
{
    int tileClickedId { ClickPos(mouseX, mouseY, tilePos, tileSize) };

    if(tileClickedId > -1 && tileClickedId < tileState.size() && tileState[tileClickedId] == 0)
        tileState[tileClickedId] = 2;
    else if(tileClickedId > -1 && tileClickedId < tileState.size() && tileState[tileClickedId] == 2)
        tileState[tileClickedId] = 0;

    return tileState;
}
