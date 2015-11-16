# Measure block access time
While working on another example, I've noticed that access time to some
monero blocks, especially early ones, is very long. Thus, I wanted measure
the time of all blocks in the blockchain.

As a result of this, this example was created showing how to loop through all
blocks in lmdb blockchain, and save some basic information from the blocks,
as well as measure access time, into an output csv file.
