/*
 * Draw the connecting passages
 *
 * passages.c	1.4 (A.I. Design)	12/14/84
 */

#include "rogue.h"
#include "curses.h"

/*
 * conn:
 *	Draw a corridor from a room in a certain direction.
 */
void
connect_rooms(first_room_index, second_room_index)
	int first_room_index, second_room_index;
{
	struct room *source_room, *destination_room = NULL;
	register int destination_index, source_index;
	int distance = 0, turn_spot, turn_distance;
	int direction;
	Position step, position, turn_delta, start_position, end_position;

	if (first_room_index < second_room_index) {
		source_index = first_room_index;
		if (first_room_index + 1 == second_room_index)
			direction = 'r';
		else
			direction = 'd';
	} else {
		source_index = second_room_index;
		if (second_room_index + 1 == first_room_index)
			direction = 'r';
		else
			direction = 'd';
	}
	source_room = &rooms[source_index];
	/*
	 * Set up the movement variables, in two cases:
	 * first drawing one down.
	 */
	if (direction == 'd') {
		destination_index = source_index + 3;				/* room # of dest */
		destination_room = &rooms[destination_index];			/* room pointer of dest */
		step.x = 0;				/* direction of move */
		step.y = 1;
		/*
		 * If we are drawing from/to regular or maze rooms, we have
		 * to pick the spot we draw from/to
		 */
		if ((source_room->flags & ROOM_ABSENT) == 0 || (source_room->flags & ROOM_MAZE)) {
			start_position.y = source_room->origin.y + source_room->size.y - 1;
			do {
				start_position.x = source_room->origin.x + random_below(source_room->size.x - 2) + 1;
			} while (terrain_at(start_position.y,start_position.x) == ' ');
		} else {
			start_position.x = source_room->origin.x;
			start_position.y = source_room->origin.y;
		}
		end_position.y = destination_room->origin.y;
		if ((destination_room->flags & ROOM_ABSENT) == 0 || (destination_room->flags & ROOM_MAZE)) {
			do {
				end_position.x = destination_room->origin.x + random_below(destination_room->size.x - 2) + 1;
			} while (terrain_at(end_position.y,end_position.x) == ' ');
		} else
			end_position.x = destination_room->origin.x;
		distance = abs(start_position.y - end_position.y) - 1;	/* distance to move */
		turn_delta.y = 0;			/* direction to turn */
		turn_delta.x = (start_position.x < end_position.x ? 1 : -1);
		turn_distance = abs(start_position.x - end_position.x);	/* how far to turn */
	} else if (direction == 'r') {			/* setup for moving right */
		destination_index = source_index + 1;
		destination_room = &rooms[destination_index];
		step.x = 1;
		step.y = 0;
		if ((source_room->flags & ROOM_ABSENT) == 0 || (source_room->flags & ROOM_MAZE)) {
			start_position.x = source_room->origin.x + source_room->size.x-1;
			do {
				start_position.y = source_room->origin.y + random_below(source_room->size.y-2)+1;
			} while (terrain_at(start_position.y,start_position.x) == ' ');
		} else {
			start_position.x = source_room->origin.x;
			start_position.y = source_room->origin.y;
		}
		end_position.x = destination_room->origin.x;
		if ((destination_room->flags & ROOM_ABSENT) == 0 || (destination_room->flags & ROOM_MAZE)) {
			do {
				end_position.y = destination_room->origin.y + random_below(destination_room->size.y-2)+1;
			} while (terrain_at(end_position.y, end_position.x) == ' ');
		} else
			end_position.y = destination_room->origin.y;
		distance = abs(start_position.x - end_position.x) - 1;
		turn_delta.y = (start_position.y < end_position.y ? 1 : -1);
		turn_delta.x = 0;
		turn_distance = abs(start_position.y - end_position.y);
	}
#ifdef DEBUG
	else
		debug("error in connection tables");
#endif
	turn_spot = random_below(distance-1) + 1;
	/*
	 * Draw in the doors on either side of the passage or just put #'s
	 * if the rooms are gone.
	 */
	if (!(source_room->flags & ROOM_ABSENT))
		place_door(source_room, &start_position);
	else
		carve_passage_cell(start_position.y, start_position.x);
	if (destination_room && !(destination_room->flags & ROOM_ABSENT))
		place_door(destination_room, &end_position);
	else
		carve_passage_cell(end_position.y, end_position.x);
	/*
	 * Get ready to move...
	 */
	position.x = start_position.x;
	position.y = start_position.y;
	while (distance)
	{
	/*
	 * Move to new position
	 */
	position.x += step.x;
	position.y += step.y;
	/*
	 * Check if we are at the turn place, if so do the turn
	 */
	if (distance == turn_spot)
	{
		while (turn_distance--)
		{
		carve_passage_cell(position.y, position.x);
		position.x += turn_delta.x;
		position.y += turn_delta.y;
		}
	}
	/*
	 * Continue digging along
	 */
	carve_passage_cell(position.y, position.x);
	distance--;
	}
	position.x += step.x;
	position.y += step.y;
	if (!positions_equal(position, end_position)) {
	end_position.x -= step.x;
	end_position.y -= step.y;
	carve_passage_cell(end_position.y, end_position.x);
	}
}

/*
 * do_passages:
 *	Draw all the passages on a level.
 */
void
generate_passages()
{
	register int i, j;
	int connected_rooms;
	static struct rdes
	{
	char	connect_rooms[MAXROOMS];		/* possible to connect to room i? */
	char	isconn[MAXROOMS];	/* connection been made to room i? */
	char	ingraph;		/* this room in graph already? */
	} room_graph[MAXROOMS] = {
	{ { 0, 1, 0, 1, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 1, 0, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 0, 1, 0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 1, 0, 0, 0, 1, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 0, 1, 0, 1, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 0, 0, 1, 0, 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 0, 0, 0, 1, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 0, 0, 0, 0, 1, 0, 1, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
	{ { 0, 0, 0, 0, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 }
	};
	struct rdes *source_node, *destination_node = NULL;

	/*
	 * reinitialize room graph description
	 */
	for (source_node = room_graph; source_node < &room_graph[MAXROOMS]; source_node++)
	{
		for (j = 0; j < MAXROOMS; j++)
			source_node->isconn[j] = FALSE;
		source_node->ingraph = FALSE;
	}

	/*
	 * starting with one room, connect it to a random adjacent room and
	 * then pick a new room to start with.
	 */
	connected_rooms = 1;
	source_node = &room_graph[random_below(MAXROOMS)];
	source_node->ingraph = TRUE;
	do
	{
		/*
		 * find a room to connect with
		 */
		j = 0;
		for (i = 0; i < MAXROOMS; i++)
			if (source_node->connect_rooms[i] && !room_graph[i].ingraph && random_below(++j) == 0)
				destination_node = &room_graph[i];
		/*
		 * if no adjacent rooms are outside the graph, pick a new room
		 * to look from
		 */
		if (j == 0)
		{
			do
				source_node = &room_graph[random_below(MAXROOMS)];
			while (!source_node->ingraph);
		}
		/*
		 * otherwise, connect new room to the graph, and draw a tunnel
		 * to it
		 */
		else
		{
			destination_node->ingraph = TRUE;
			i = source_node - room_graph;
			j = destination_node - room_graph;
			connect_rooms(i, j);
			source_node->isconn[j] = TRUE;
			destination_node->isconn[i] = TRUE;
			connected_rooms++;
		}
	} while (connected_rooms < MAXROOMS);

	/*
	 * attempt to add passages to the graph a random number of times so
	 * that there isn't always just one unique passage through it.
	 */
	for (connected_rooms = random_below(5); connected_rooms > 0; connected_rooms--)
	{
		source_node = &room_graph[random_below(MAXROOMS)];	/* a random room to look from */
		/*
		 * find an adjacent room not already connected
		 */
		j = 0;
		for (i = 0; i < MAXROOMS; i++)
			if (source_node->connect_rooms[i] && !source_node->isconn[i] && random_below(++j) == 0)
				destination_node = &room_graph[i];
		/*
		 * if there is one, connect it and look for the next added
		 * passage
		 */
		if (j != 0)
		{
			i = source_node - room_graph;
			j = destination_node - room_graph;
			connect_rooms(i, j);
			source_node->isconn[j] = TRUE;
			destination_node->isconn[i] = TRUE;
		}
	}
	number_passages();
}


/*
 * door:
 *	Add a door or possibly a secret door.  Also enters the door in
 *	the exits array of the room.
 */
void
place_door(room, position)
	struct room *room;
	Position *position;
{
	register int index, exit_index;

	index = map_index(position->y, position->x);
	if (random_below(10) + 1 < dungeon_level && random_below(5) == 0)
	{
		terrain_map[index] = (position->y == room->origin.y || position->y == room->origin.y + room->size.y - 1) ? HWALL : VWALL;
		cell_flags[index] &= ~CELL_REVEALED;
	}
	else
		terrain_map[index] = DOOR;
	exit_index = room->exit_count++;
	room->exits[exit_index].y = position->y;
	room->exits[exit_index].x = position->x;
}

//@ Unused function
#ifdef WIZARD
/*
 * add_pass:
 *	Add the passages to the current window (wizard command)
 */
void
add_pass()
{
	register int y, x, ch;

	for (y = 1; y < dungeon_bottom_row; y++)
		for (x = 0; x < COLS; x++)
			if ((ch = terrain_at(y, x)) == DOOR || ch == PASSAGE)
				mvaddch(y, x, ch);
}
#endif

/*
 * passnum:
 *	Assign a number to each passageway
 */
static int passage_number;
static byte passage_needs_number;

void
number_passages()
{
	register struct room *room;
	register int i;

	passage_number = 0;
	passage_needs_number = FALSE;
	for (room = passages; room < &passages[MAXPASS]; room++)
		room->exit_count = 0;
	for (room = rooms; room < &rooms[MAXROOMS]; room++)
		for (i = 0; i < room->exit_count; i++)
		{
			passage_needs_number++;
			mark_connected_passage(room->exits[i].y, room->exits[i].x);
		}
}
/*
 * numpass:
 *	Number a passageway square and its brethren
 */
void
mark_connected_passage(y, x)
	int y, x;
{
	register byte *flags_cursor;
	register struct room *room;
	register byte character;

	if (outside_dungeon(y,x))
		return;
	flags_cursor = &cell_flags_at(y, x);
	if (*flags_cursor & PASSAGE_NUMBER_MASK)
		return;
	if (passage_needs_number) {
		passage_number++;
		passage_needs_number = FALSE;
	}
	/*
	 * check to see if it is a door or secret door, i.e., a new exit,
	 * or a numerable type of place
	 */
	if ((character = terrain_at(y, x)) == DOOR || (!(*flags_cursor & CELL_REVEALED) && character != FLOOR)) {
		room = &passages[passage_number];
		room->exits[room->exit_count].y = y;
		room->exits[room->exit_count++].x = x;
	} else if (!(*flags_cursor & CELL_PASSAGE))
		return;
	*flags_cursor |= passage_number;
	/*
	 * recurse on the surrounding places
	 */
	mark_connected_passage(y + 1, x);
	mark_connected_passage(y - 1, x);
	mark_connected_passage(y, x + 1);
	mark_connected_passage(y, x - 1);
}

void
carve_passage_cell(y, x)
	shint y, x;
{
	register int idx;

	terrain_map[idx = map_index(y, x)] = PASSAGE;
	cell_flags[idx] |= CELL_PASSAGE;
}
