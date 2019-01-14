typedef enum CmdType {
	AnimMove,
	AnimTeleport,
	AnimRandWait,
} CmdType;

typedef struct AnimationCmd {
	CmdType type;
	union {
		// Move
		struct { int duration; int velocity; Direction direction; };
		// Teleport to pix
		struct { int x_px; int y_px; };
		// RandWait for max_ticks
		struct { int max_ticks; };
	};
} AnimationCmd;

typedef struct AnimationProgram {
	unsigned n_cmds;
	AnimationCmd* cmds; // Can point to heap or stack
} AnimationProgram;

typedef struct AnimationProcessor {
	int pc;
	int ticks_left;
} AnimationProcessor;

void anim_run(AnimationProcessor* apu, AnimationProgram* aprog, Car* target) {
	/* printf("apu pc: %d, tl: %d\n", apu->pc, apu->ticks_left); */
	apu->ticks_left -= 1;
	// Execute it and fetch next instruction
	if (apu->ticks_left <= 0) {
		// Fetch instruction	
		const AnimationCmd *ins = &aprog->cmds[apu->pc];
		// Execute it
		
		// Target?
		switch (ins->type) {
			case AnimMove:
				target->direction = ins->direction;
				target->velocity = ins->velocity;
				apu->ticks_left = ins->duration;
				break;
			case AnimTeleport:
				apu->ticks_left = 1;
				target->x_px = ins->x_px;
				target->y_px = ins->y_px;
				target->x = target->x_px * 1024;
				target->y = target->y_px * 1024;
				break;
			case AnimRandWait:
				target->velocity = 0;
				apu->ticks_left = random() % ins->max_ticks;
				break;
		}

		// Increment program counter and reset time_left
		apu->pc = (apu->pc + 1) % aprog->n_cmds;
	}
}

