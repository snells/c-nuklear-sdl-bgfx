import os,sys
e = Environment(CPPPATH=['include'])
e['ENV']['TERM'] = os.environ['TERM']

SetOption('num_jobs', int(os.environ.get('NUM_CPU', 2)))
e.Program('exe', Glob('src/*.c'),
LIBS=['SDL2', 'm', 'GL', 'pthread', 'dl', 'bgfx', 'X11'],
LIBPATH='lib', CCFLAGS=['-g', '-fPIC', '--std=gnu11', '-Wall', '-Wextra'])
