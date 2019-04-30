Why tho?
-------------------------
These drivers include my settings I use.

I use these settings:
-------------------------
```
# Firstly, you need to know what tablet you want to edit.
xsetwacom --list

# This is my output:
Wacom One by Wacom S Pen stylus         id: 15  type: STYLUS
Wacom One by Wacom S Pen eraser         id: 22  type: ERASER

# So, "device name" is "Wacom One by Wacom S Pen stylus" (you don't need an eraser variant, my tablet doesn't even have an eraser.)

# stylus variant                                              
xsetwacom set "Wacom One by Wacom S Pen stylus" Suppress 2 
xsetwacom set "Wacom One by Wacom S Pen stylus" RawSample 4 
```

I have dual monitor setup, so I want to make my tablet use only my first monitor.
```
# Firstly, you need to know what tablet you want to edit.
xsetwacom --list

# This is my output:
Wacom One by Wacom S Pen stylus         id: 15  type: STYLUS
Wacom One by Wacom S Pen eraser         id: 22  type: ERASER

# So, "id" is "15" (you don't need an eraser variant, my tablet doesn't even have an eraser.)

# Now, my main monitor is 1366x768, but you may have a different one. You can easily change it. Just look to it.
xsetwacom --set "15" MapToOutput 1366x768+1366+0 >> ~/.bashrc # < yes, I'm putting it in bashrc, because that's a setting, which won't be remembered.
```

How do I build it?:
-------------------------
[Here's a simple guide, but you can use package from your distro, and set these settings above in.](https://github.com/linuxwacom/xf86-input-wacom/wiki/Building-The-Driver)
