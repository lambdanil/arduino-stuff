/* 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <LiquidCrystal_I2C.h>

#define B_R 2 // buttons for direction
#define B_L 0
#define B_U 4
#define B_D 3
#define DISPLAY_H 4 // display height
#define DISPLAY_W 20 // display width
#define DISPLAY_ADDR 0x27 // I2C display address
#define MS_WAIT 300

struct pos {
  unsigned int x;
  unsigned int y;
};

typedef struct Vector {
    size_t size; // number of elements in vector
    struct pos* at; // dynamic array
} vector;

void vector_init(vector *cvector, size_t n) {
    cvector->at = (struct pos*)malloc(n * sizeof(struct pos)); // allocate memory
    cvector->size = n;
}

void vector_resize(vector *cvector, size_t n) {
    cvector->at = (struct pos*)realloc(cvector->at, n * sizeof(struct pos)); // reallocate memory
    cvector->size = n; 
}

void vector_push(vector *cvector, struct pos toPush) {
    size_t nsize = (cvector->size+1) ; // increase number of elements by one
    vector_resize(cvector, nsize);
    cvector->at[(cvector->size)-1] = toPush; // insert the value
}

void vector_insert(vector *cvector, struct pos toPush, int n) {
    size_t nsize = (cvector->size+1) ; // increase number of elements by one
    vector_resize(cvector, nsize);
    for (int i = (int)cvector->size - 1; i > n; i--) {
        cvector->at[i] = cvector->at[i-1]; // move array values
    } 
    cvector->at[n] = toPush; // insert the value
}

void vector_free(vector *cvector) {
    free(cvector->at);
}

void vector_delete_last(vector *cvector) {
    vector_resize(cvector, cvector->size - 1); // deallocate last cell
}

void vector_delete_at(vector *cvector, int n) { // delete specific value
    for (int i = n; i < (int)cvector->size - 1; i++) {
        cvector->at[i] = cvector->at[i+1]; // move array values
    }
    vector_resize(cvector, cvector->size - 1); // resize array
}

LiquidCrystal_I2C LCD(DISPLAY_ADDR, DISPLAY_W, DISPLAY_H);

String pad_text(String, String fill=" ", int max=DISPLAY_W);

#define WIDTH DISPLAY_W
#define HEIGHT DISPLAY_H
#define HEAD_CHAR 'X'
#define SEG_CHAR 'x'
#define FOOD_CHAR 'o'
#define FLOOR_CHAR ' '
#define BORDER_CHAR '#'

struct scr_buf {
  const unsigned int width = WIDTH;
  const unsigned int height = HEIGHT;
  char buf[HEIGHT][WIDTH];
};

struct game_state {
  struct scr_buf screen;
  struct pos head;
  struct pos food;
  vector segments;
  char dir; // 'u', 'l', 'd', 'r'
  char l_dir;
  bool add_segment_next_loop = false;
};

void move_food(struct game_state& game) {
  game.food.x = random(0, game.screen.width - 1);
  game.food.y = random(0, game.screen.height - 1);
  while ((game.screen.buf[game.food.y][game.food.x] != FLOOR_CHAR) ||
        (game.food.x == game.segments.at[game.segments.size-1].x &&
         game.food.y == game.segments.at[game.segments.size-1].y)) {
    game.food.x = random(0, game.screen.width - 1);
    game.food.y = random(0, game.screen.height - 1); 
  }
}

String pad_text(String text, String fill=" ", int max=DISPLAY_W) {
  String ftext = text;
  while (ftext.length() < max) {
    ftext = fill + ftext;
    ftext += fill;
  }
  return ftext;
}

void game_over(struct game_state& game) {
  LCD.setCursor(0,DISPLAY_H/2-1);
  LCD.print(pad_text("Game over!"));
  LCD.setCursor(0,DISPLAY_H/2);
  LCD.print(pad_text("Final score:" + String(game.segments.size-1)));
  exit(0);
}

void next_frame(struct game_state& game) {
  struct scr_buf& screen = game.screen;
  bool& add_segment_next_loop = game.add_segment_next_loop;
  static struct pos last_segment;
  static bool first_run = true;

  if (game.segments.size) {
    struct pos new_segment;
    new_segment.x = game.segments.at[game.segments.size - 1].x;
    new_segment.y = game.segments.at[game.segments.size - 1].y;
    for (int i = game.segments.size - 1; i > 0; i--) {
      game.segments.at[i] = game.segments.at[i - 1];
    }
    game.segments.at[0] = game.head;
    if (add_segment_next_loop) {
      vector_push(&game.segments, new_segment);
      add_segment_next_loop = false;
    }
  }

  if (game.head.x == game.food.x && game.head.y == game.food.y) {
    add_segment_next_loop = true;
    move_food(game);
  }

  switch (game.dir) {
    case 'u':
      if (game.head.y == 0)
        game_over(game);
      game.head.y--;
      break;
    case 'd':
      if (game.head.y == game.screen.height - 1)
        game_over(game);
      game.head.y++;
      break;
    case 'r':
      if (game.head.x == game.screen.width - 1)
        game_over(game);
      game.head.x++;
      break;
    case 'l':
      if (game.head.x == 0)
        game_over(game);
      game.head.x--;
      break;
  }

  if (game.segments.size) {
    if (first_run) {
      first_run = false;
    }

    if (!game.add_segment_next_loop) {
      screen.buf[last_segment.y][last_segment.x] = FLOOR_CHAR;
      last_segment.x = game.segments.at[game.segments.size - 1].x;
      last_segment.y = game.segments.at[game.segments.size - 1].y;
    }

    screen.buf[game.segments.at[0].y][game.segments.at[0].x] = SEG_CHAR;
    screen.buf[game.segments.at[game.segments.size-1].y][game.segments.at[game.segments.size-1].x] = SEG_CHAR;
  }

  if (!(game.head.x == game.segments.at[0].x && game.head.y == game.segments.at[0].y)
      &&
      screen.buf[game.head.y][game.head.x] == SEG_CHAR && !first_run)
    game_over(game);

  if (game.head.x == game.food.x && game.head.y == game.food.y) {
    add_segment_next_loop = true;
    move_food(game);
  }
}

void fill_screen(struct scr_buf& screen, char fill) {
  for (unsigned int i = 0; i < screen.height; i++) {
    for (unsigned int j = 0; j < screen.width; j++) {
      screen.buf[i][j] = fill;
    }
  }
}

void fill_segments(struct game_state& game) {
  for (int i = game.segments.size - 1; i > 0; i--) {
    game.screen.buf[game.segments.at[i].y][game.segments.at[i].x] = SEG_CHAR;
  }
}

void init_game(struct game_state& game) {
  game.head.x = game.screen.width / 2;
  game.head.y = game.screen.height / 2;
  fill_screen(game.screen, FLOOR_CHAR);
  fill_segments(game);
  move_food(game);
  struct pos first_segment;
  first_segment.x = game.head.x;
  first_segment.y = game.head.y;
  vector_init(&game.segments, 0);
  vector_push(&game.segments, first_segment);
}

void print_screen(struct game_state& game) {
  struct scr_buf& screen = game.screen;
  String out = "";

  LCD.setCursor(0, 0);
  int li;
  for (unsigned int i = 0; i < screen.height; i++) {
    if (li != i) {
      LCD.setCursor(0, li);
      LCD.print(out);
      out = "";
    }
    for (unsigned int j = 0; j < screen.width; j++) {
      if (i == game.head.y && j == game.head.x)
        out += HEAD_CHAR;
      else if (i == game.food.y && j == game.food.x)
        out += FOOD_CHAR;
      else if (screen.buf[i][j] == SEG_CHAR)
        out += SEG_CHAR;
      else
        out += screen.buf[i][j];
    }
    li = i;
  }
  LCD.setCursor(0, li);
  LCD.print(out);
} 

void eval_key(struct game_state& game) {
    if (!digitalRead(B_R) && game.l_dir != 'l') {
      game.dir = 'r';
    }
    if (!digitalRead(B_L) && game.l_dir != 'r') {
      game.dir = 'l';
    }
    if (!digitalRead(B_U) && game.l_dir != 'd') {
      game.dir = 'u';
    }
    if (!digitalRead(B_D) && game.l_dir != 'u') {
      game.dir = 'd';
    }
}

struct game_state game;

void setup() {
  pinMode(B_R, INPUT_PULLUP);
  pinMode(B_L, INPUT_PULLUP);
  pinMode(B_D, INPUT_PULLUP);
  pinMode(B_U, INPUT_PULLUP);
  LCD.begin(DISPLAY_W, DISPLAY_H);
  LCD.backlight();
  LCD.noBlink();
  randomSeed(analogRead(A0));
  //srand(time(0));
  init_game(game);

  char mv;
}

void loop() {
  static int counter = 0;

  if(counter == MS_WAIT) {
    next_frame(game);
    print_screen(game);
    game.l_dir = game.dir;
  }

  eval_key(game);

  counter++;
  if (counter > MS_WAIT) counter = 0;
  delay(1);
}
