ffmpeg -i %1 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 9 "%~n1.mjpeg"
ffmpeg -i %1 -f u16le -acodec pcm_u16le -ar 44100 -ac 1 "%~n1.pcm"
pause