# Application

The main repository embeds an example application which is able to run on the Vanilla kernel. The application can be programmed the same way as any other application. 

# Options 

The application.c file in the root directory offers some configuration options. Currently the options are

- NAME - name of the application. This will show in the runtime statistics
- STACK_SIZE - requested stack size in number of words (4-bytes)
- SCHEDULER - the requested scheduler which will schedule the application

# Some things to consider

The application should not write to registers. Register accesses are currently not thread-safe and will not be guaranteed to work. The kernel is based on a hierarchical multiclass scheduler. That means that any scheduled real-time thread will run before any application or background thread. If the user makes a real-time thread which never sleeps or waits it will allways be in the running queue, thus blocking all execution in the lower priority scheduling classes completely. 

It is better to allocate to much stack than to little. A stack size of 256 bytes or 0x100 bytes is suited for most applications which is not allocating memory inside functions. Remember to use the mm_alloc memory allocator, this will not increase the stack usage much. 

If adding multiple file to the application these must also be added to the makefile in order for them to be compiled. 
