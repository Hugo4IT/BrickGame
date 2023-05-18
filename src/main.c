#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
  uint8_t rotation;
  PieceRepresentation repr_cache;
} Piece;

const PieceRotationDescriptor PT_J_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b10000000,
            0b11100000,
            0b00000000,
        },
        {
            0b00000000,
            0b01100000,
            0b01000000,
            0b01000000,
        },
        {
            0b00000000,
            0b00000000,
            0b11100000,
            0b00100000,
        },
        {
            0b00000000,
            0b01000000,
            0b01000000,
            0b11000000,
        },
    },
};

const PieceRotationDescriptor PT_L_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b00100000,
            0b11100000,
            0b00000000,
        },
        {
            0b00000000,
            0b01000000,
            0b01000000,
            0b01100000,
        },
        {
            0b00000000,
            0b00000000,
            0b11100000,
            0b10000000,
        },
        {
            0b00000000,
            0b11000000,
            0b01000000,
            0b01000000,
        },
    },
};

const PieceRotationDescriptor PT_T_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b01000000,
            0b11100000,
            0b00000000,
        },
        {
            0b00000000,
            0b01000000,
            0b01100000,
            0b01000000,
        },
        {
            0b00000000,
            0b00000000,
            0b11100000,
            0b01000000,
        },
        {
            0b00000000,
            0b01000000,
            0b11000000,
            0b01000000,
        },
    },
};

const PieceRotationDescriptor PT_I_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b00000000,
            0b11110000,
            0b00000000,
        },
        {
            0b00100000,
            0b00100000,
            0b00100000,
            0b00100000,
        },
        {
            0b00000000,
            0b00000000,
            0b11110000,
            0b00000000,
        },
        {
            0b01000000,
            0b01000000,
            0b01000000,
            0b01000000,
        },
    },
};

const PieceRotationDescriptor PT_S_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b01100000,
            0b11000000,
            0b00000000,
        },
        {
            0b00000000,
            0b01000000,
            0b01100000,
            0b00100000,
        },
        {
            0b00000000,
            0b00000000,
            0b01100000,
            0b11000000,
        },
        {
            0b00000000,
            0b10000000,
            0b11000000,
            0b01000000,
        },
    },
};

const PieceRotationDescriptor PT_Z_ROTATIONS = {
    4,
    {
        {
            0b00000000,
            0b11000000,
            0b01100000,
            0b00000000,
        },
        {
            0b00000000,
            0b00100000,
            0b01100000,
            0b01000000,
        },
        {
            0b00000000,
            0b00000000,
            0b11000000,
            0b01100000,
        },
        {
            0b00000000,
            0b01000000,
            0b11000000,
            0b10000000,
        },
    },
};

const PieceRotationDescriptor PT_O_ROTATIONS = {
    1,
    {
        {
            0b00000000,
            0b01100000,
            0b01100000,
            0b00000000,
        },
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    },
};

const OptionalTileColor NONE = {
  .is_some = false,
  .value = BLUE,
};

const SDL_Color TEXT_COLOR = {0xF8, 0xF9, 0xFA, 255};
const SDL_Color BG_COLOR = {0x21, 0x25, 0x29, 255};
const SDL_Color BOARD_COLOR = {0x49, 0x50, 0x57, 255};
SDL_Rect tile_rect = {0, 0, 48, 48};
SDL_Rect board_rect = {0, 0, 48 * 8, 48 * 16};

SDL_TimerID falling_piece_timer;

Piece falling_piece;
int32_t falling_piece_x = 0;
int32_t falling_piece_y = 0;
uint32_t falling_piece_interval = 800;

Piece piece_queue[3];
Piece *held_piece = NULL;

uint8_t board[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

bool paused = false;

OptionalTileColor visual_board[16][8] = {
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
  { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE },
};

uint64_t score = 0;

uint8_t safe_shl(uint8_t value, int sh) {
  if (sh < 0)
    return value >> -sh;
  else if (sh > 0)
    return value << sh;
  return value;
}

uint8_t safe_shr(uint8_t value, int sh) {
  if (sh < 0)
    return value << -sh;
  else if (sh > 0)
    return value >> sh;
  return value;
}

const SDL_Color TILE_FILL[7] = {
  [LIGHT_BLUE] = {0x22, 0xB8, 0xCF, 255},
  [YELLOW] = {0xFC, 0xC4, 0x19, 255},
  [PINK] = {0xF0, 0x65, 0x95, 255},
  [BLUE] = {0x33, 0x9A, 0xF0, 255},
  [ORANGE] = {0xFF, 0x92, 0x2B, 255},
  [GREEN] = {0x51, 0xCF, 0x66, 255},
  [RED] = {0xFF, 0x6B, 0x6B, 255},
};

const SDL_Color TILE_OUTLINE[7] = {
  [LIGHT_BLUE] = {0x10, 0x98, 0xAD, 255},
  [YELLOW] = {0xF5, 0x9F, 0x00, 255},
  [PINK] = {0xD6, 0x33, 0x6C, 255},
  [BLUE] = {0x1C, 0x7E, 0xE6, 255},
  [ORANGE] = {0xF7, 0x67, 0x07, 255},
  [GREEN] = {0x37, 0xB2, 0x4D, 255},
  [RED] = {0xF0, 0x3E, 0x3E, 255},
};

const PieceRotationDescriptor ROTATION_DESCRIPTORS[] = {
    [PT_I] = PT_I_ROTATIONS, [PT_O] = PT_O_ROTATIONS, [PT_T] = PT_T_ROTATIONS,
    [PT_J] = PT_J_ROTATIONS, [PT_L] = PT_L_ROTATIONS, [PT_S] = PT_S_ROTATIONS,
    [PT_Z] = PT_Z_ROTATIONS,
};

bool check_collision(PieceRepresentation repr, int32_t x, int32_t y) {
  for (int i = 0; i < 4; i++) {
    if (!repr[i])
      continue;

    int real_y = y + i;

    if (real_y < 0 || real_y >= 16 || safe_shl(safe_shr(repr[i], x), x) != repr[i] ||
        (board[real_y] & safe_shr(repr[i], x)))
      return true;
  }

  return false;
}

Piece new_piece(enum PieceType type) {
  Piece piece = {
    .type = type,
    .rotation = 0,
    .repr_cache = { 0, 0, 0, 0 },
  };

  memcpy(&piece.repr_cache, &ROTATION_DESCRIPTORS[piece.type].rotations[0], 4);

  return piece;
}

void rotate_piece(Piece *piece) {
  PieceRotationDescriptor rotation = ROTATION_DESCRIPTORS[piece->type];

  if (!check_collision(
          rotation.rotations[(piece->rotation + 1) % rotation.count],
          falling_piece_x, falling_piece_y)) {
    piece->rotation = (piece->rotation + 1) % rotation.count;
    memcpy(&piece->repr_cache, &rotation.rotations[piece->rotation], 4);
  }
}

void render_tile(SDL_Renderer *renderer, int32_t x, int32_t y,
                 enum TileColor tile_color, uint8_t opacity) {
  SDL_Color fill = TILE_FILL[tile_color];
  SDL_Color outline = TILE_OUTLINE[tile_color];

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

  SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, opacity);
  SDL_RenderFillRect(renderer, &fill_rect);

  SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, opacity);
  SDL_RenderFillRect(renderer, &outline_rect);
}

void render_piece(SDL_Renderer *renderer, int32_t x, int32_t y, Piece piece, uint8_t opacity) {
  for (int ty = 0; ty < 4; ty++) {
    uint8_t row = piece.repr_cache[ty];
    for (int tx = 0; tx < 4; tx++) {
      if (row & (0b10000000 >> tx)) {
        render_tile(renderer, tx + x, ty + y, (enum TileColor)piece.type, opacity);
      }
    }
  }
}

void render_text(SDL_Renderer *renderer, int32_t x, int32_t y, TTF_Font *font,
                 const char *text, SDL_Color color) {
  SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect rect;
  rect.x = x;
  rect.y = y;
  rect.w = surface->w;
  rect.h = surface->h;

  SDL_RenderCopy(renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void render_text_centered(SDL_Renderer *renderer, int32_t x, int32_t y, TTF_Font *font,
                          const char *text, SDL_Color color) {
  SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect rect;
  rect.x = x - surface->w / 2;
  rect.y = y - surface->h / 2;
  rect.w = surface->w;
  rect.h = surface->h;

  SDL_RenderCopy(renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

bool try_move(int32_t delta_x, int32_t delta_y) {
  if (!check_collision(falling_piece.repr_cache, falling_piece_x + delta_x,
                       falling_piece_y + delta_y)) {
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

  falling_piece_x = 1;
  falling_piece_y = 0;
}

void remove_line(int y) {
  board[y] = 0;

  for (int i = 0; i < 8; i++) {
    visual_board[y][i].is_some = false;
  }
}

void move_line(int src_y, int dest_y) {
  int coll = board[src_y];
  OptionalTileColor visual[8];

  memcpy(&visual, &visual_board[src_y], sizeof(OptionalTileColor) * 8);

  remove_line(src_y);

  board[dest_y] = coll;
  memcpy(&visual_board[dest_y], &visual, sizeof(OptionalTileColor) * 8);
}

void check_board() {
  int chain = 0;
  for (int i = 0; i < 16; i++) {
    if (board[i] == 0xFF) {
      remove_line(i);

      for (int j = 0; j < i - 1; j++)
        move_line(i - j - 1, i - j);
      
      score += 10;

      chain++;
    } else {
      if (chain == 3) {
        score += 25;
      } else if (chain == 4) {
        score += 50;
      } else if (chain > 4) {
        score += 50 + chain * 10;
      }

      chain = 0;
    }
  }
}

uint32_t on_tick(uint32_t _interval, void *_param) {
  if (paused)
    return 5;
  
  if (!try_move(0, 1)) {
    // Solidify falling piece
    for (int i = 0; i < 4; i++) {
      board[falling_piece_y + i] |=
          safe_shr(falling_piece.repr_cache[i], falling_piece_x);
    }

    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (falling_piece.repr_cache[y] &
            (0b10000000 >> x)) {
          visual_board[y + falling_piece_y][x + falling_piece_x].is_some = true;
          visual_board[y + falling_piece_y][x + falling_piece_x].value =
              (enum TileColor)falling_piece.type;
        }
      }
    }

    // Generate new falling piece
    pop_queue();

    check_board();
  }
  
  uint32_t interval = falling_piece_interval - (score / 10);

  if (interval < 1)
    interval = 1;
  
  return interval;
}

int main() {
  srand(time(0));

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "Error: Couldn't initialize SDL2: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  SDL_Window *window = SDL_CreateWindow(
      "BrickGame", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 800,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
  if (window == NULL) {
    fprintf(stderr, "Error: Couldn't initialize a window: %s\n",
            SDL_GetError());
    exit(EXIT_FAILURE);
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (window == NULL) {
    fprintf(stderr, "Error: Couldn't initialize a renderer: %s\n",
            SDL_GetError());
    exit(EXIT_FAILURE);
  }

  TTF_Init();
  TTF_Font *roboto = TTF_OpenFont("res/Roboto-Regular.ttf", 32);
  if (!roboto) {
    fprintf(stderr, "Error: Couldn't load font: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  falling_piece = new_piece((enum PieceType)(rand() % 7));
  falling_piece_timer = SDL_AddTimer(falling_piece_interval, on_tick, NULL);
  piece_queue[0] = new_piece((enum PieceType)(rand() % 7));
  piece_queue[1] = new_piece((enum PieceType)(rand() % 7));
  piece_queue[2] = new_piece((enum PieceType)(rand() % 7));

  bool running = true;
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
#ifdef __APPLE__
    board_rect.x = window_width - board_rect.w / 2;
    board_rect.y = window_height - board_rect.h / 2;
#else
    board_rect.x = window_width / 2 - board_rect.w / 2;
    board_rect.y = window_height / 2 - board_rect.h / 2;
#endif
    SDL_SetRenderDrawColor(renderer, BOARD_COLOR.r, BOARD_COLOR.g,
                           BOARD_COLOR.b, 255);
    SDL_RenderFillRect(renderer, &board_rect);

    // Board FG
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 8; x++) {
        if (visual_board[y][x].is_some) {
          render_tile(renderer, x, y, visual_board[y][x].value, 255);
        }
      }
    }

    // Falling piece
    render_piece(renderer, falling_piece_x, falling_piece_y, falling_piece, 255);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Render ghost
    for (int i = 0; i < 16; i++) {
      if (check_collision(falling_piece.repr_cache, falling_piece_x, falling_piece_y + i)) {
        int y = falling_piece_y + i - 1;

        render_piece(renderer, falling_piece_x, y, falling_piece, 40);

        break;
      }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    char score_text[25];
    if (score <= 9999999999999999)
      sprintf(&score_text[0], "Score: %lu", score);
    
    render_text(renderer, 5, 5, roboto, score_text, TEXT_COLOR);

    if (paused) {
      SDL_Rect screen_rect;
      SDL_GetWindowSize(window, &screen_rect.w, &screen_rect.h);
      SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 255);
      SDL_RenderFillRect(renderer, &screen_rect);
      render_text_centered(renderer, screen_rect.w / 2, screen_rect.h / 2, roboto, "paused", TEXT_COLOR);
    }

    SDL_RenderPresent(renderer);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
        break;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_p) {
          paused = !paused;
          continue;
        }

        if (paused)
          continue;

        switch (event.key.keysym.sym) {
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
        case SDLK_SPACE:
        case SDLK_x:
          for (int i = 0; i < 16; i++)
            try_move(0, 1);
          on_tick(0, NULL);
          break;
        default:
          break;
        }
      }
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  TTF_Quit();

  return EXIT_SUCCESS;
}