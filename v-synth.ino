#include <ESP_8_BIT_composite.h>

ESP_8_BIT_composite videoOut(true);

const uint8_t input_pot_pin = 34;
const uint8_t input_button_pin = 33;

const int CELL_SIZE_X = 4;
const int CELL_SIZE_Y = 4;

const int PIXELS_X = 256;
const int PIXELS_Y = 240;

const int CELLS_X = PIXELS_X / CELL_SIZE_X;
const int CELLS_Y = PIXELS_Y / CELL_SIZE_Y;

const int CELL_LIFETIME = 3;

// int born_rule[] = {3};
// int* survive_rule[] = {2, 3};

uint8_t board[CELLS_Y * CELLS_X];
uint8_t board_copy[CELLS_Y * CELLS_X];

void IRAM_ATTR isr() {
  for (int y = CELLS_Y / 2 - 5; y < CELLS_Y / 2 + 5; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void setup() {
  pinMode(input_button_pin, INPUT_PULLUP);
  attachInterrupt(input_button_pin, isr, FALLING);
  analogReadResolution(9);

  videoOut.begin();

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void render(uint8_t** frameBufferLines) {
  for (int y = 0; y < PIXELS_Y; y++) {
    for (int x = 0; x < PIXELS_X; x++) {
      frameBufferLines[y][x] = board[y * CELLS_Y + x] * 255 / CELL_LIFETIME;
    }
  }

  videoOut.waitForFrame();

  //  for (int y = 0; y < PIXELS_Y; y++) {
  //     for (int x = 0; x < PIXELS_X; x++) {
  //       frameBufferLines[y][x] =
  //           board[(y / CELL_SIZE_Y) * CELLS_Y + (x / CELL_SIZE_X)] *
  //           color_multiplier / CELL_LIFETIME;
  //     }
  //   }
}

void loop() {
  uint8_t** frameBufferLines = videoOut.getFrameBufferLines();
  int color_multiplier = map(analogRead(input_pot_pin), 0, 511, 0, 255);

  render();
  evolve();

  delay(16);
}

void evolve() {
  memcpy(board_copy, board, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      uint8_t total_n = 0;

      for (int dx = -1; dx < 2; dx++) {
        for (int dy = -1; dy < 2; dy++) {
          if (!(dx == 0 && dy == 0)) {
            if (board[((y + dy) % CELLS_Y) * CELLS_Y + ((x + dx) % CELLS_X)] ==
                CELL_LIFETIME - 1) {
              total_n += 1;
            }
          }
        }
      }

      if (total_n == 3) {
        board_copy[y * CELLS_Y + x] = CELL_LIFETIME - 1;
      }

      if (total_n > 3 || total_n < 2 && board_copy[y * CELLS_Y + x] > 0) {
        board_copy[y * CELLS_Y + x] -= 1;
      }
    }
  }

  memcpy(board, board_copy, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  return;
}
