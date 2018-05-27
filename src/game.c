#include <string.h>
#include <game.h>
#include <stdlib.h>

char field[FIELD_SIZE_Y + 2][FIELD_SIZE_X + 2] = {{0, }};

static char fig[4][4] = {{0, }};
static const unsigned short figs[] = {
		0x0f00, 0x4444, 0x0f00, 0x4444,
		0x0e80, 0x0622, 0x02e0, 0x4460,
		0x08e0, 0x0644, 0x0e20, 0x2260,
		0x0c60, 0x0264, 0x0c60, 0x0264,
		0x06c0, 0x8c40, 0x06c0, 0x8c40,
		0x0660, 0x0660, 0x0660, 0x0660};

static int fig_x;
static int fig_y;
static char fig_type;
static int rotation;
int game_state;
int tick_count;
int score;

static void init_fig() {
	memset(fig, 0, sizeof(fig));
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			fig[i][j] = ((figs[(fig_type - 1) * 4 + (rotation & 3)] >> (i << 2)) & (1 << j)) ? fig_type : 0;
		}
	}
}

static void paint_fig() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (fig[i][j]) {
				field[fig_y + i][fig_x + j] = fig_type;
			}
		}
	}
}

static void clear_fig() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (fig[i][j]) {
				field[fig_y + i][fig_x + j] = 0;
			}
		}
	}
}

static int check_collision() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (fig[i][j]) {
				if (field[fig_y + i][fig_x + j] != 0) return 1;
			}
		}
	}
	return 0;
}

static void next() {
	fig_x = 1;
	fig_y = 1;
	rotation = 0;
	fig_type = (rand() % 6) + 1;
    init_fig();
    if (check_collision()) {
    		game_state = GAME_OVER;
    } else {
    		paint_fig();
    }
}

static void shift(int row) {
	while (row > 0) {
		for (int i = 1; i <= FIELD_SIZE_X; i++) {
			field[row][i] = field[row - 1][i];
		}
		row--;
	}
}

static void check_lines() {
	int row = FIELD_SIZE_Y;
	while (row > 0) {
		int full = 1;
		for (int i = 1; i <= FIELD_SIZE_X; i++) {
			if (!field[row][i]) {
				full = 0;
				break;
			}
		}
		if (full) {
			shift(row);
			score += 10;
		} else {
			row--;
		}
	}
}

void rotate() {
	clear_fig();
	rotation++;
	init_fig();
	if (check_collision()) {
		rotation--;
		init_fig();
	}
	paint_fig();
}

void down() {
	while(1) {
		clear_fig();
		fig_y++;
		if (check_collision()) {
			fig_y--;
			paint_fig();
			check_lines();
			next();
			break;
		} else {
			paint_fig();
		}	
	}
}

void move_right() {
    clear_fig();
    fig_x++;
    if (check_collision()) {
		fig_x--;
	}
	paint_fig();
}

void move_left() {
    clear_fig();
    fig_x--;
    if (check_collision()) {
		fig_x++;
	}
	paint_fig();
}

void model_init() {
	memset(field, 0, sizeof(field));
	game_state = GAME_PLAYING;
	tick_count = 0;
	score = 0;
	for (int i = 0; i < FIELD_SIZE_Y + 2; i++) {
		field[i][0] = 7;
		field[i][FIELD_SIZE_X + 1] = 7;
	}
	for (int i = 1; i <= FIELD_SIZE_X; i++) {
		field[FIELD_SIZE_Y + 1][i] = 7;
	}
	next();
}

void model_tick() {
	tick_count++;
	if (game_state == GAME_OVER) {
		model_init();
		return;
	}
	if (tick_count % 6 != 0) return;
	clear_fig();
	fig_y++;
	if (check_collision()) {
		fig_y--;
		paint_fig();
		check_lines();
		next();
	} else {
		paint_fig();
	}
}



