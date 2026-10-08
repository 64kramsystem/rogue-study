/*
 * Maze drawing routines.  Based on the algorithm presented in the
 * December 1981 Byte "How to Build a Maze" by David Matuszek.
 *
 * maze.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"
#include "screen.h"

#define MAX_MAZE_FRONTIER 100

#define FRONTIER 'F'
#define NOTHING ' '

static shint frontier_count, selected_frontier_y, selected_frontier_x, maze_top, maze_left;
static shint maze_right, maze_bottom;
static shint *frontier_rows, *frontier_columns;

void
draw_maze(struct room *room)
{
	register int y, x;
	shint frontier_y_storage[MAX_MAZE_FRONTIER], frontier_x_storage[MAX_MAZE_FRONTIER];
	int neighbor_mask;
	Position loop_position;

	frontier_rows = frontier_y_storage;
	frontier_columns = frontier_x_storage;
	maze_right = maze_bottom = 0;
	maze_top = room->origin.y;
	if (maze_top == 0)
		maze_top = ++room->origin.y;
	maze_left = room->origin.x;
	/*
	 * Choose a random spot in the maze and initialize the frontier
	 * to be the immediate neighbors of this random spot.
	 */
	y = maze_top;
	x = maze_left;
	carve_maze_cell(y,x);
	expand_maze_frontier(y, x);
	/*
	 * While there are new frontiers, connect them to the path and
	 * possibly expand the frontier even more.
	 */
	while(frontier_count)
	{
		connect_maze_frontier();
		expand_maze_frontier(selected_frontier_y, selected_frontier_x);
	}
	/*
	 * According to the Grand Beeking, every maze should have a loop
	 * Don't worry if you don't understand this.
	 */
	room->size.x = maze_right - room->origin.x + 1;
	room->size.y = maze_bottom - room->origin.y + 1;
	do {
		static Position neighbor_directions[4] = {
			{-1,  0},
			{ 0,  1},
			{ 1,  0},
			{ 0, -1}
		};
		Position *direction;
		int direction_bit;

		random_room_position(room, &loop_position);
		for (neighbor_mask = 0,direction = neighbor_directions,direction_bit = 1; direction < &neighbor_directions[4]; direction_bit <<= 1,direction++) {
			y = direction->y + loop_position.y; x = direction->x + loop_position.x;
			if (!outside_dungeon(y, x) && terrain_at(y, x) == PASSAGE)
				neighbor_mask += direction_bit;
		}
	} while (terrain_at(loop_position.y, loop_position.x) == PASSAGE || neighbor_mask % 5);
	carve_maze_cell(loop_position.y, loop_position.x);
}

void
expand_maze_frontier(int y, int x)
{
	add_maze_frontier(y-2, x);
	add_maze_frontier(y+2, x);
	add_maze_frontier(y, x-2);
	add_maze_frontier(y, x+2);
}

void
add_maze_frontier(int y, int x)
{
#ifdef DEBUG
	if (frontier_count == MAX_MAZE_FRONTIER - 1)
		debug("MAZE DRAWING ERROR #3\n");
#endif
	if (inside_maze_bounds(y, x) && terrain_at(y, x) == NOTHING)
	{
		terrain_at(y, x) = FRONTIER;
		frontier_rows[frontier_count] = y;
		frontier_columns[frontier_count++] = x;
	}
}

/*
 * Connect randomly to one of the adjacent points in the spanning tree
 */
void
connect_maze_frontier(void)
{
	register int frontier_index, direction_index, y_step = 0, x_step = 0;
	int available_directions[4];
	int direction_count = 0, y, x;


	/*
	 * Choose a random frontier
	 */
	frontier_index = random_below(frontier_count);
	selected_frontier_y = frontier_rows[frontier_index];
	selected_frontier_x = frontier_columns[frontier_index];
	frontier_rows[frontier_index] = frontier_rows[frontier_count-1];
	frontier_columns[frontier_index] = frontier_columns[--frontier_count];

	/*
	 * Count and collect the adjacent points we can connect to
	 */
	if (is_maze_passage(selected_frontier_y-2, selected_frontier_x))
		available_directions[direction_count++] = 0;
	if (is_maze_passage(selected_frontier_y+2, selected_frontier_x))
		available_directions[direction_count++] = 1;
	if (is_maze_passage(selected_frontier_y, selected_frontier_x-2))
		available_directions[direction_count++] = 2;
	if (is_maze_passage(selected_frontier_y, selected_frontier_x+2))
		available_directions[direction_count++] = 3;
	/*
	 * Choose one of the open places, connect to it and
	 * then the task is complete
	 */
	direction_index = available_directions[random_below(direction_count)];
	carve_maze_cell(selected_frontier_y, selected_frontier_x);
	switch(direction_index)
	{
	case 0:
		direction_index = 1;
		y_step = -1;
		break;
	case 1:
		direction_index = 0;
		y_step = 1;
		break;
	case 2:
		direction_index = 3;
		x_step = -1;
		break;
	case 3:
		direction_index = 2;
		x_step = 1;
		break;
	}
	y = selected_frontier_y + y_step;
	x = selected_frontier_x + x_step;
	if (inside_maze_bounds(y, x))
		carve_maze_cell(y, x);
}

bool
is_maze_passage(int y, int x)
{
	return (inside_maze_bounds(y, x) && terrain_at(y, x) == PASSAGE);
}

void
carve_maze_cell(int y, int x)
{
	terrain_at(y, x) = PASSAGE;
	cell_flags_at(y, x) = CELL_MAZE|CELL_REVEALED;
	if (x > maze_right)
		maze_right = x;
	if (y > maze_bottom)
		maze_bottom = y;
}

#define MAZE_Y_LIMIT (maze_top+((dungeon_bottom_row+1)/3))
#define MAZE_X_LIMIT (maze_left+COLS/3)

bool
inside_maze_bounds(int y, int x)
{
	return(y >= maze_top && y < MAZE_Y_LIMIT && x >= maze_left && x < MAZE_X_LIMIT);
}
