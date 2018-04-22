#!/bin/bash

qsub -o 1proc.txt edge2image1.pbs
qsub -o 2proc.txt edge2image2.pbs
qsub -o 4proc.txt edge2image4.pbs
qsub -o 8proc.txt edge2image8.pbs
qsub -o 16proc.txt edge2image16.pbs
qsub -o 24proc.txt edge2image24.pbs
qsub -o 48proc.txt edge2image48.pbs
qsub -o 72proc.txt edge2image72.pbs
qsub -o 96proc.txt edge2image96.pbs
qsub -o 120proc.txt edge2image120.pbs



