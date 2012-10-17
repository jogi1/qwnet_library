#!/bin/bash
spawn ()
{
    ./a.out $1 $2 &
}
for i in {20710..20718}; do spawn $i $1; done
