#!/bin/bash

qsub -o /output/1proc.txt edge2image1.pbs
qsub -o /output/2proc.txt edge2image2.pbs
qsub -o /output/4proc.txt edge2image4.pbs
qsub -o /output/8proc.txt edge2image8.pbs
qsub -o /output/16proc.txt edge2image16.pbs
qsub -o /output/24proc.txt edge2image24.pbs
qsub -o /output/48proc.txt edge2image48.pbs
qsub -o /output/72proc.txt edge2image72.pbs
qsub -o /output/96proc.txt edge2image96.pbs
qsub -o /output/120proc.txt edge2image120.pbs



