#include <ESP_8_BIT_composite.h>
#include <SimpleKalmanFilter.h>

const uint8_t input_pot_1_pin = 34;
const uint8_t input_pot_2_pin = 35;
const uint8_t input_pot_3_pin = 32;
const uint8_t input_pot_4_pin = 33;

const int CELL_SIZE_X = 2;
const int CELL_SIZE_Y = 2;

const int PIXELS_X = 256;
const int PIXELS_Y = 240;

const int CELL_LIFETIME = 7;

const int CELLS_X = PIXELS_X / CELL_SIZE_X;
const int CELLS_Y = PIXELS_Y / CELL_SIZE_Y;

const uint8_t STATE_DEAD = 0;
const uint8_t STATE_ALIVE = CELL_LIFETIME - 1;

ESP_8_BIT_composite video_out(true);

SimpleKalmanFilter filter_1(1, 1, 0.01);

uint16_t born_rule = 0b000001000;     // {3}
uint16_t survive_rule = 0b000001100;  // {3,2}

uint8_t board[CELLS_Y * CELLS_X];
uint8_t board_copy[CELLS_Y * CELLS_X];

void render(uint8_t** frameBufferLines, int color_multiplier) {
  for (int y = 0; y < PIXELS_Y; y++) {
    for (int x = 0; x < PIXELS_X; x++) {
      int index = (y / CELL_SIZE_Y) * CELLS_Y + (x / CELL_SIZE_X);

      frameBufferLines[y][x] = board[index] * color_multiplier / CELL_LIFETIME;
    }
  }

  video_out.waitForFrame();
}

void generate_center_line(uint8_t thickness) {
  int y_from = CELLS_Y / 2 - 1 * thickness;
  int y_to = CELLS_Y / 2 + 1 * thickness;

  for (int y = y_from; y < y_to; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void setup() {
  analogReadResolution(9);

  video_out.begin();

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void loop() {
  uint8_t** frame_buffer = video_out.getFrameBufferLines();
  int estimated_value = filter_1.updateEstimate(analogRead(input_pot_1_pin));

  int color_multiplier = map(estimated_value, 0, 511, 0, 255);

  born_rule = analogRead(input_pot_2_pin);
  survive_rule = analogRead(input_pot_3_pin);

  int center_line_force = map(analogRead(input_pot_4_pin), 0, 511, 0, 15);

  render(frame_buffer, color_multiplier);

  evolve();

  if (center_line_force > 0) {
    generate_center_line(center_line_force);
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
          if (dx != 0 || dy != 0) {
            int neighborX = (x + dx) % CELLS_X;
            int neighborY = (y + dy) % CELLS_Y;

            int neighbor_state = board[neighborY * CELLS_Y + neighborX];

            if (neighbor_state == STATE_ALIVE) {
              total_n += 1;
            }
          }
        }
      }

      if (current_state == STATE_DEAD) {
        if ((born_rule >> total_n) & 1) {
          board_copy[y * CELLS_Y + x] = STATE_ALIVE;
        }
      } else if (current_state == STATE_ALIVE) {
        if (!((survive_rule >> total_n) & 1)) {
          board_copy[y * CELLS_Y + x] = STATE_DEAD;
        }
      } else {
        board_copy[y * CELLS_Y + x] = current_state - 1;
      }
    }
  }

  memcpy(board, board_copy, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  return;
}
