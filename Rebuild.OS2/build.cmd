@SETLOCAL
@rem !!! build final version !!! 
SET GCCOPT= -Zomf -s -Zcrtdll -Zlinker /PM:VIO -Zlinker /EXE:2 
@rem !!! build debug version !!! SET GCCOPT= -g  
@rem !!! not used : vms.c vms.h files.c exclude.c !!!
GCC -DUSE_SCG -Zstack 1024 -o mkisofs.exe mkisofs.c eltorito.c fnmatch.c getopt.c getopt1.c hash.c joliet.c match.c multi.c name.c rock.c tree.c write.c cdaccess.c
LXLITE mkisofs.exe
