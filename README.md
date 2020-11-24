# **SLAP**  

This is essentially a poorly made git copy. There are a few differences between how Git and Slap operate, which I will explain in more detail below. 

## <u>**VERSION 0.0.0**</u>  
Version 0.0.0 (v0.0.0) was released on November 24, 2020.  
**I predict that subsequent versions will be incompatible with v0.0.0.**

### <u>**USAGE**</u>  
So far, Slap has only four commands:
* **init** - initializes an empty Slap repository in the working directory
* **add <files\>** - adds a file to the repository. <files\> cannot be a directory
* **commit** - creates a commit
* **checkout <commit path\>** - checks out the commit located at <commit path\>  

### <u>**DETAILS**</u>
A slap repository, like a git repository, is just a directory in your file system. The name of this directory is .slap . **slap init** creates this directory and all essential subdirectories and files.  

When you **add** a file to the repository, it takes that file and adds it to the objects directory of the repository. Files in the objects directory are called blobs. This blob is put into its own subdirectory whose name is the first byte of its sha hash in hex. The name of the blob file are the remaining 19 bytes in hex.  
So if you have a file whose hash is 2d8723fda77194ed155a6868241c4789cf02d4db, its path (relative to the working directory) is: `.slap/objects/2d/8723fda77194ed155a6868241c4789cf02d4db`  
A file segment is also added to the index file. An index file segment has the sha of the file in repository, the sha of the file in the last commit, and the sha of the file in the working directory. These shas can be used to see if a commit will be up-to-date. The segment also has the path and mode of the file.

**commit**ting creates a new blob that has the shas of previous commits and takes the index and strips out the shas for the working dir and staging area (non-committed blobs) from the index file.

**checkout**-ing a commit takes a commit blob and reconstructs the working directory according to it.

### <u>**NOTES**</u>
As of v0.0.0, Slap does not have branches. This will hopefully change.  
Slap probably has a couple of bugs that I am not aware of, if you find any, please create a bug report.  

**Thank you for using (or even just visiting) Slap!**