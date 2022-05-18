#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

enum TileColor {
    LIGHT_BLUE,
    YELLOW,
    PINK,
    BLUE,
    ORANGE,
    GREEN,
    RED,
};
typedef struct OptionalTileColor {
    bool is_some;
    enum TileColor value;
} OptionalTileColor;

enum PieceType {
    PT_I,
    PT_O,
    PT_T,
    PT_J,
    PT_L,
    PT_S,
    PT_Z,
};

typedef uint8_t PieceRepresentation[4];

typedef struct PieceRotationDescriptor {
    uint8_t count;
    PieceRepresentation rotations[4];
} PieceRotationDescriptor;

typedef struct Piece {
    enum PieceType type;
    enum TileColor color;
    uint8_t rotation;
    PieceRepresentation repr_cache;
} Piece;

const PieceRotationDescriptor PT_J_ROTATIONS = {
    4,
    {
        {
            0b01000000,
            0b01000000,
            0b11000000,
            0b00000000,
        },
        {
            0b00000000,
            0b11100000,
            0b00100000,
            0b00000000,
        },
        {
            0b01100000,
            0b01000000,
            0b01000000,
            0b00000000,
        },
        {
            0b10000000,
            0b11100000,
            0b00000000,
            0b00000000,
        },
    },
};

const PieceRotationDescriptor PT_L_ROTATIONS = {
    4,
    {
        {
            0b11000000,
            0b01000000,
            0b01000000,
            0b00000000,
        },
        {
            0b00000000,
            0b11100000,
            0b10000000,
            0b00000000,
        },
        {
            0b01000000,
            0b01000000,
            0b01100000,
            0b00000000,
        },
        {
            0b00100000,
            0b11100000,
            0b00000000,
            0b00000000,
        },
    },
};

const PieceRotationDescriptor PT_T_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b11100000,
            0b01000000,
            0b00000000,
        },
        {
            0b01000000,
            0b01100000,
            0b01000000,
            0b00000000,
        },
        {
            0b01000000,
            0b11100000,
            0b00000000,
            0b00000000,
        },
        {
            0b01000000,
            0b11000000,
            0b01000000,
            0b00000000,
        },
    },
};

const PieceRotationDescriptor PT_I_ROTATIONS = {
    2,
    {
        {
            0b00000000,
            0b11110000,
            0b00000000,
            0b00000000,
        },
        {
            0b01000000,
            0b01000000,
            0b01000000,
            0b01000000,
        },
        {0,0,0,0},
        {0,0,0,0},
    },
};

const PieceRotationDescriptor PT_S_ROTATIONS = {
    2,
    {
        {
            0b01100000,
            0b11000000,
            0b00000000,
            0b00000000,
        },
        {
            0b10000000,
            0b11000000,
            0b01000000,
            0b00000000,
        },
        {0,0,0,0},
        {0,0,0,0},
    },
};

const PieceRotationDescriptor PT_Z_ROTATIONS = {
    2,
    {
        {
            0b11000000,
            0b01100000,
            0b00000000,
            0b00000000,
        },
        {
            0b01000000,
            0b11000000,
            0b10000000,
            0b00000000,
        },
        {0,0,0,0},
        {0,0,0,0},
    },
};

const PieceRotationDescriptor PT_O_ROTATIONS = {
    1,
    {
        {
            0b11000000,
            0b11000000,
            0b00000000,
            0b00000000,
        },
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
    },
};

const SDL_Color TEXT_COLOR = { 0x0D, 0x22, 0x26, 255 };
const SDL_Color BG_COLOR = { 0xE0, 0xE9, 0xF5, 255 };
const SDL_Color BOARD_COLOR = { 0xC9, 0xD7, 0xE8, 255 };
SDL_Rect tile_rect = { 0, 0, 48, 48 };
SDL_Rect board_rect = { 0, 0, 48 * 8, 48 * 16 };

SDL_TimerID falling_piece_timer;

Piece falling_piece;
int32_t falling_piece_x;
int32_t falling_piece_y;
uint32_t falling_piece_interval = 800;

Piece piece_queue[3];
Piece held_piece;

uint8_t board[16];
OptionalTileColor visual_board[8][16];

// Debug stats
uint64_t tick;
uint64_t frame;

void copy_piece_representation(PieceRepresentation* src, PieceRepresentation* dest) {
    for (int i = 0; i < 4; i++)
        ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
}

void get_tile_color(enum TileColor tile_color, SDL_Color* fill, SDL_Color* outline) {
    switch (tile_color) {
    case LIGHT_BLUE:
        *fill = (SDL_Color){ 0x42, 0xD0, 0xFF, 255 };
        *outline = (SDL_Color){ 0x21, 0xBA, 0xED, 255 };
        break;
    case YELLOW:
        *fill = (SDL_Color){ 0xF8, 0xED, 0x49, 255 };
        *outline = (SDL_Color){ 0xEB, 0xDE, 0x52, 255 };
        break;
    default:
        *fill = (SDL_Color){ 255, 0, 0, 255 };
        *outline = (SDL_Color){ 255, 0, 0, 255 };
        break;
    }
}

enum TileColor get_piece_color(enum PieceType type) {
    switch(type) {
    case PT_I: return LIGHT_BLUE;
    case PT_O: return YELLOW;
    case PT_T: return PINK;
    case PT_J: return BLUE;
    case PT_L: return ORANGE;
    case PT_S: return GREEN;
    case PT_Z: return RED;
    }
}

PieceRotationDescriptor get_rotation_descriptor(Piece* piece) {
    switch (piece->type) {
    case PT_I: return PT_I_ROTATIONS;
    case PT_O: return PT_O_ROTATIONS;
    case PT_T: return PT_T_ROTATIONS;
    case PT_J: return PT_J_ROTATIONS;
    case PT_L: return PT_L_ROTATIONS;
    case PT_S: return PT_S_ROTATIONS;
    case PT_Z: return PT_Z_ROTATIONS;
    }
}

bool check_collision(PieceRepresentation repr, int32_t x, int32_t y) {
    int32_t piece_offset_x = 3;
    int32_t piece_width;
    int32_t piece_height;
    for (int i = 0; i < 4; i++) {
        if (repr[i]) piece_height = i + 1;
        if (piece_offset_x > 2 && repr[i] & 0b00100000) piece_offset_x = 2;
        if (piece_offset_x > 1 && repr[i] & 0b01000000) piece_offset_x = 1;
        if (piece_offset_x > 0 && repr[i] & 0b10000000) piece_offset_x = 0;
        if (piece_width < 1 && repr[i] & 0b10000000) piece_width = 1;
        if (piece_width < 2 && repr[i] & 0b01000000) piece_width = 2;
        if (piece_width < 3 && repr[i] & 0b00100000) piece_width = 3;
        if (piece_width < 4 && repr[i] & 0b00010000) piece_width = 4;
    }
    
    int32_t h_start = x + piece_offset_x;
    int32_t h_end = x + piece_width;
    if (y < 0 || y + piece_height > 16) return true;
    if (h_start < 0 || h_end > 8) return true;

    for (int32_t i = 0; i < piece_height; i++) {
        if (board[y + i] & (repr[i] >> x)) return true;
    }

    return false;
}

Piece new_piece(enum PieceType type) {
    Piece piece;
    piece.type = type;
    piece.rotation = 0;
    piece.color = get_piece_color(type);
    copy_piece_representation(&get_rotation_descriptor(&piece).rotations[0], &piece.repr_cache);
    return piece;
}

void rotate_piece(Piece* piece) {
    PieceRotationDescriptor rotation = get_rotation_descriptor(piece);
    if (!check_collision(rotation.rotations[(piece->rotation + 1) % rotation.count], falling_piece_x, falling_piece_y)) {
        piece->rotation = (piece->rotation + 1) % rotation.count;
        copy_piece_representation(&rotation.rotations[piece->rotation], &piece->repr_cache);
    }
}

void render_tile(SDL_Renderer* renderer, int32_t x, int32_t y, enum TileColor tile_color) {
    SDL_Color fill;
    SDL_Color outline;
    get_tile_color(tile_color, &fill, &outline);

    SDL_Rect fill_rect;
    fill_rect.x = board_rect.x + tile_rect.w * x;
    fill_rect.y = board_rect.y + tile_rect.h * y;
    fill_rect.w = tile_rect.w;
    fill_rect.h = tile_rect.h;

    SDL_Rect outline_rect = fill_rect;
    outline_rect.x += fill_rect.w / 8;
    outline_rect.y += fill_rect.h / 8;
    outline_rect.w -= fill_rect.w / 4;
    outline_rect.h -= fill_rect.h / 4;

    SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, 255);
    SDL_RenderFillRect(renderer, &fill_rect);

    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, 255);
    SDL_RenderFillRect(renderer, &outline_rect);
}

void render_piece(SDL_Renderer* renderer, int32_t x, int32_t y, Piece piece) {
    for (int ty = 0; ty < 4; ty++) {
        uint8_t row = piece.repr_cache[ty];
        for (int tx = 0; tx < 4; tx++)
            if (row & (0b10000000 >> tx))
                render_tile(renderer, tx + x, ty + y, piece.color);
    }
}

void render_text(SDL_Renderer* renderer, int32_t x, int32_t y, TTF_Font* font, const char* text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

bool try_move(int32_t delta_x, int32_t delta_y) {
    if (!check_collision(falling_piece.repr_cache, falling_piece_x + delta_x, falling_piece_y + delta_y)) {
        falling_piece_x += delta_x;
        falling_piece_y += delta_y;
        return true;
    }

    return false;
}

void pop_queue() {
    falling_piece = piece_queue[0];
    piece_queue[0] = piece_queue[1];
    piece_queue[1] = piece_queue[2];
    piece_queue[2] = new_piece((enum PieceType)(rand() % 7));
    
    falling_piece_x = 2;
    falling_piece_y = 0;
}

uint32_t on_tick(uint32_t interval, void* param) {
    if (!try_move(0, 1)) {
        // Solidify falling piece
        for (int i = 0; i < 16 - falling_piece_y; i++) {
            board[falling_piece_y + i] |= falling_piece.repr_cache[i] >> falling_piece_x;
        }

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (board[falling_piece_y + y] & (0b10000000 >> x)) {
                    visual_board[y + falling_piece_y][x + falling_piece_x].is_some = true;
                    visual_board[y + falling_piece_y][x + falling_piece_x].value = falling_piece.color;
                }
            }
        }

        // Generate new falling piece
        pop_queue();
    }

    tick++;
    return falling_piece_interval;
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Error: Couldn't initialize SDL2: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window* window = SDL_CreateWindow("BrickGame", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Error: Couldn't initialize a window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (window == NULL) {
        fprintf(stderr, "Error: Couldn't initialize a renderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    TTF_Init();
    TTF_Font* roboto = TTF_OpenFont("res/Roboto-Regular.ttf", 32);
    if (!roboto) {
        fprintf(stderr, "Error: Couldn't load font: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    falling_piece = new_piece(PT_T);
    falling_piece_timer = SDL_AddTimer(falling_piece_interval, on_tick, NULL);

    bool running = true;
    bool debug_info = false;
    uint64_t frame = 0;
    while (running) {
        int window_width;
        int window_height;
        SDL_GetWindowSize(window, &window_width, &window_height);

        // Background
        SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 255);
        SDL_RenderClear(renderer);

        // Board BG
        board_rect.w = tile_rect.w * 8;
        board_rect.h = tile_rect.h * 16;
        board_rect.x = window_width - board_rect.w / 2;
        board_rect.y = window_height - board_rect.h / 2;
        SDL_SetRenderDrawColor(renderer, BOARD_COLOR.r, BOARD_COLOR.g, BOARD_COLOR.b, 255);
        SDL_RenderFillRect(renderer, &board_rect);

        // Board FG
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 8; x++) {
                if (visual_board[y][x].is_some) {
                    render_tile(renderer, x, y, visual_board[y][x].value);
                }
            }
        }

        // Falling piece
        render_piece(renderer, falling_piece_x, falling_piece_y, falling_piece);

        if (debug_info) {
            char* frame_text;
            asprintf(&frame_text, "Frame: %llu, Tick: %llu", frame, tick);
            render_text(renderer, 10, 10, roboto, frame_text, TEXT_COLOR);
            free(frame_text);

            char board_text[9];
            board_text[8] = '\0';
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 8; x++) {
                    if (board[y] & (0b10000000 >> x))
                        board_text[x] = '1';
                    else
                        board_text[x] = '0';
                }
                render_text(renderer, 10, 46 + 32 * y, roboto, board_text, TEXT_COLOR);
            }
        }

        SDL_RenderPresent(renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_F3:
                    debug_info = !debug_info;
                    break;
                case SDLK_UP:
                    rotate_piece(&falling_piece);
                    break;
                case SDLK_LEFT:
                    try_move(-1, 0);
                    break;
                case SDLK_RIGHT:
                    try_move(1, 0);
                    break;
                case SDLK_DOWN:
                    try_move(0, 1);
                    break;
                default: break;
                }
            }
        }
        frame++;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();

    return EXIT_SUCCESS;
}