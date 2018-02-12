# Sunset Run - GameOff2017 (Not submitted)
Sunset Run is a race game, stylized as the old line/vector display consoles.
All the rendering is made line by line in a software renderer (except the background and some UI text), with some blur and scanlines applied to mimic the old screens. As it was not finished in time for the jam, it wasn't submitted.

![alt text](https://github.com/KellerMartins/GameOff2017/blob/master/Textures/Screenshots.png "Screenshots")

### Tools Used
The game was programmed in C, using the SDL2 library with the SDL2_image, SDL2_ttf and [SDL_FontCache](https://github.com/grimfang4/SDL_FontCache) extensions. The logo and background images were made in Gimp, and the 3D graphics were made in Blender and exported to the .txt files with some scripts.

### State of the game: UNFINISHED AND BUGGY
Sadly I couldn't finish the game in time, as I got busy with other things by the middle of the jam and a friend who was helping had to drop it, but it still was a lot fun to create, and helped me to improve my skills in C and in the SDL API.
Later, I fixed some problems that made the game unplayable (like the lack of the finish line...), but it's still very buggy and the code is very messy (as expected from something made in a jam), but if you are curious to try you can compile it by modifying the makefile to find the correct path to the libs (should work on Windows and Linux).
