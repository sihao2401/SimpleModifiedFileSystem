# Simple Modified File System from UNIX V6
Unix V6 file system has a current limitation of 16MB on file size. Redesign the file system and remove this limitation. New file system support files up to 4GB in size. Also, the block size is 1024 Bytes, all fields of Super Block are now doubled and the free[] array is expanded so that the information stored in the super block is closer to 1024 Bytes.

### The modified file system support the following command:
* initfs will initialize the file system.

* Quit the program by saving all the work.

* Implement move function.

* Implement check free block function.

* cpin external file 
(create new v6-file in the modified file system.)
* cpout v6-file externalfile
(create externalfile and make its contents equal to v6-file.)
* mkdir v6-dir
(create the directory.)
* rm v6-dir
(delete the file.)
* q
(save all changes and quit.)

