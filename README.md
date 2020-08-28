# audio_fish
Audio wave visualizer. 
In a convo about writing GUIs using C/C++ being a bad time, i jokingly said 'just write the GUI using python and send real time data over a socket'.  

That's basically how this experiment was born. I wanted to see if i could get any good results visualizing real time data.  
Turns out you sorta can, although this approach it not optimal.
It's easy for the visualisation to lag behind the audio playback.  
Tweaking params like the buffersize of data being sent to the client helps, and with more optimization i'm pretty sure you can get better results then i got here.

## main.c
Uses alsa to playback audio.  
Also starts server in the background to send this audio data.

## server.c
Sends audio data to the visualisation client. 
Size of each send is defined in config.txt

## visual_client.py
Uses matplotlib to plot the audiowave
