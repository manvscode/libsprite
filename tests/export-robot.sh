#!/bin/bash



./bin/sprc -c "Robot" \
	-t 70 \
    -a walk:./tests/robot/walk0.tga \
    -a walk:./tests/robot/walk1.tga \
    -a walk:./tests/robot/walk2.tga \
    -a walk:./tests/robot/walk3.tga \
    \
    -a run:./tests/robot/running0.tga \
    -a run:./tests/robot/running1.tga \
    -a run:./tests/robot/running2.tga \
    \
    -a run:./tests/robot/running0.tga \
    -a run:./tests/robot/running1.tga \
    -a run:./tests/robot/running2.tga \
	\
    -a jump:./tests/robot/jump0.tga \
    -a jump:./tests/robot/jump1.tga \
    -a jump:./tests/robot/jump2.tga \
	\
    -a climb:./tests/robot/climb0.tga \
    -a climb:./tests/robot/climb1.tga \
    -a climb:./tests/robot/climb2.tga \
    -a climb:./tests/robot/climb3.tga \
	-x
	
