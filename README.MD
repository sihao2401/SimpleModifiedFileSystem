Unix V6 file system has a current limitation of 16MB on file size. Redesign the file system and remove this limitation. New file system support files up to 4GB in size. Also, the block size is 1024 Bytes, all fields of Super Block are now doubled and the free[] array is expanded so that the information stored in the super block is closer to 1024 Bytes.
The modified file system support the following command:
(a)
initfs will initialize the file system.
(b)
Quit the program by saving all the work.
(c)
Implement move function.
(d)
Implement check free block function.