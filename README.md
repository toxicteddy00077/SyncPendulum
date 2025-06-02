# SyncPendulum
![](https://github.com/toxicteddy00077/SyncPendulum/blob/main/Assets/ezgif.com-video-to-gif-converter.gif)

## Frameworks used: 
OpenCL, GLUT, zlib(for packet compression)

This is a Double Pendulum or Chaos Pendulum simulation, which leverages **dsitibuted compute** by offloading all calculations to a secondary device(Raspberry Pi), which returns the updated state back to the host machine(in my case a docker container), which finally renders the new state in realtime.

I have used UDP packets to send bundled states to my Raspberry Pi(server.c runs on the serving machine), which extracts the state, performs euler integration for the new state, and sends the updated compressed state back to the host, which renders it. This enables the host to get cheaper rendering on less powerful devices such as **integrated GPUs**, since all computations will be handled by the secondary device. Any latency introdcued can be amortized by batching and sending the states, and increasing complexity of computation.

### To Run:
    g++ -o main /SyncPendulum/OnDevice/main.cpp -lGLEW -lglfw -lGL -lGLU -lglut
    ./main
