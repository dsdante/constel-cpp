#define BUFF_SIZE 256

static float fps_history[BUFF_SIZE];
static int count = 0;
static int pointer = 0;

// Get mean FPS among the last [frame] values
float get_fps(int frame)
{
    if (count == 0)
        return 0;
    if (frame < 1)
        frame = 1;
    if (frame > count)
        frame = count;
    float sum = 0;
    for (int i = (pointer - frame + BUFF_SIZE) % BUFF_SIZE; i != pointer; i = (i+1) % BUFF_SIZE)
        sum += fps_history[i];
    return sum / frame;
}

// Get mean FPS for the last [period] seconds, according to the last FPS value
float get_fps_period(float period)
{
    return get_fps(period * fps_history[(pointer-1+BUFF_SIZE)%BUFF_SIZE]);
}

// Append a new FPS value
void set_fps(float value)
{
    fps_history[pointer] = value;
    pointer++;
    pointer %= BUFF_SIZE;
    if (count < BUFF_SIZE)
        count++;
}
