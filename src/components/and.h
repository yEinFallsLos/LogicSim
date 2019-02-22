#pragma once
#include "component.h"
#include "output.h"
#include "input.h"
#include "link.h"

class AND :
	public Component
{
public:
	AND(Board* board, Input** inputs, Output** outputs, int inputCount) : Component(board, inputs, outputs, inputCount, 1) { }
	AND(Board* board, Link** inputs, Link** outputs, int inputCount) : Component(board, inputs, outputs, inputCount, 1) { }

	unsigned int getMinInputCount() { return 1; }
	unsigned int getMaxInputCount() { return UINT_MAX; }
	unsigned int getMinOutputCount() { return 1; }
	unsigned int getMaxOutputCount() { return 1; }

	void compute() {
		for (int i = 0; i < inputCount; i++) {
			if (!inputs[i]->getPowered()) {
				outputs[0]->setPowered(false);
				return;
			}
			outputs[0]->setPowered(true);
		}
	}
};

