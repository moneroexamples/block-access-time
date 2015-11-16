# Measure block access time
While working on another example, I've noticed that access time to some
monero blocks, especially early ones, using `Blockchain::get_block_by_hash`,
is very long. Thus, I wanted check this for all blocks  blockchain.

As a result of this, this example was created showing how to loop through all
blocks in lmdb blockchain, and save some basic information from the blocks,
as well as measure access time, into an output csv file.

# C++: main.cpp
