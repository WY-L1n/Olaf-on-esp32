#Olaf-master/src
olaf_db_mem_pack.h
- Establish a new format for storing fingerprints, which stores their 64-bits hash, time and audio ID

olaf_db_mem.c
- Songs database management (sort and query)

olaf_fp_db_writer_mem.h
- Enable every fingerprint generated on the computer being stored in the same database

olaf_fp_ref_mem.h
- Generated fingerprint database for ESP32 (it is empty now, please read instructions below)

#custom path
* Replace src/olaf_fp_ref_mem.h temporarily, to test when only one fingerprint is used for recognition
olaf_39_generated.h
- Used to store fingerprint from 39 original audios (songs by kanye west)

olaf_single_generated.h
- Temporarily used to store fingerprint from only one original audio (Never see me again)

olaf_test_300.h
- Temporarily used to store fingerprint from only one original audio (530)

#others
partitions.csv
- An available partition scheme to use