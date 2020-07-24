#!/bin/bash

for exp in experiment?
do
    for config in $exp/*
    do
        for trace in sample*large*.tr
        do
            ./CPU_cache $trace $config 0
        done
    done
done
