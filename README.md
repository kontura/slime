# Slime

Create source file:
```
ffmpeg -i input.m4a -vn -ar 44100 -ac 2 -b:a 192k output.mp2
```

TODO:
* create simple frequency visualization (or perhaps something like `music_visualizer`)
* Take a look into chaotic attractors for particle paths (https://www.youtube.com/watch?v=idpOunnpKTo)
* Try out fluid sim: https://www.youtube.com/watch?v=qsYE1wMEMPA
    - https://www.karlsims.com/fluid-flow.html
    - https://www.dgp.toronto.edu/public_user/stam/reality/Research/pdf/ns.pdf
