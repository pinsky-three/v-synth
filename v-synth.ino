#include <ESP_8_BIT_composite.h>
#include <SimpleKalmanFilter.h>

const uint8_t input_pot_pin = 34;
const uint8_t input_pot_2_pin = 35;
const uint8_t input_button_pin = 33;

const int CELL_SIZE_X = 2;
const int CELL_SIZE_Y = 2;

const int PIXELS_X = 256;
const int PIXELS_Y = 240;

const int CELL_LIFETIME = 2;

const int CELLS_X = PIXELS_X / CELL_SIZE_X;
const int CELLS_Y = PIXELS_Y / CELL_SIZE_Y;

ESP_8_BIT_composite video_out(true);
SimpleKalmanFilter kalman_filter(5, 5, 0.01);

uint16_t born_rule = 0b000001000;     // {3}
uint16_t survive_rule = 0b000001100;  // {3,2}

uint8_t board[CELLS_Y * CELLS_X];
uint8_t board_copy[CELLS_Y * CELLS_X];

void render(uint8_t** frameBufferLines, int color_multiplier) {
  for (int y = 0; y < PIXELS_Y; y++) {
    for (int x = 0; x < PIXELS_X; x++) {
      frameBufferLines[y][x] =
          board[(y / CELL_SIZE_Y) * CELLS_Y + (x / CELL_SIZE_X)] *
          color_multiplier / CELL_LIFETIME;
    }
  }

  video_out.waitForFrame();
}

void generate_center_line() {
  for (int y = CELLS_Y / 2 - 5; y < CELLS_Y / 2 + 5; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void setup() {
  pinMode(input_button_pin, INPUT_PULLUP);
  analogReadResolution(9);

  video_out.begin();

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void loop() {
  uint8_t** frameBufferLines = video_out.getFrameBufferLines();
  int estimated_value = kalman_filter.updateEstimate(analogRead(input_pot_pin));

  int color_multiplier = map(estimated_value, 0, 511, 0, 255);

  render(frameBufferLines, color_multiplier);
  evolve();

  if (!digitalRead(input_button_pin)) {
    generate_center_line();
  }

  delay(16);
}

void evolve() {
  memcpy(board_copy, board, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      int current_state = board[y * CELLS_Y + x];

      uint8_t total_n = 0;

      for (int dx = -1; dx < 2; dx++) {
        for (int dy = -1; dy < 2; dy++) {
          if (!(dx == 0 && dy == 0)) {
            int neighbor_state =
                board[((y + dy) % CELLS_Y) * CELLS_Y + ((x + dx) % CELLS_X)];

            if (neighbor_state == CELL_LIFETIME - 1) {
              total_n += 1;
            }
          }
        }
      }

      // if (total_n > 0) {
      //   total_n -= 1;
      // }

      if (current_state == 0) {
        if ((born_rule >> total_n) & 1) {
          board_copy[y * CELLS_Y + x] = CELL_LIFETIME - 1;
        }
      } else {
        if ((survive_rule >> total_n) & 1) {
          board_copy[y * CELLS_Y + x] = current_state;
        } else {
          board_copy[y * CELLS_Y + x] = current_state - 1;
        }
      }
    }
  }

  memcpy(board, board_copy, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  return;
}
