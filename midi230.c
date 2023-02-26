#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *KEYWORD = "noteblock_harp";
const int PITCH_OFFSET = 66;
const unsigned short MAX_VOLUME = 100;
const char CUTOFF_VEL = 0;

short readbig_i16(FILE *f)
{
    short n;
    for (int i = 0; i < 2; ++i)
    {
        n <<= 8;
        fread(&n, 1, 1, f);
    }
    return n;
}

unsigned int readbig_ui32(FILE *f)
{
    unsigned int n;
    for (int i = 0; i < 4; ++i)
    {
        n <<= 8;
        fread(&n, 1, 1, f);
    }
    return n;
}

enum eventtype
{
    NOTE_ON,
    TEMPO,
    START,
    TRACK_NAME
};

struct event
{
    unsigned long tick;
    enum eventtype type;
    struct event *next;
    void *data;
};

struct track
{
    char *keyword;
    int offset;
    unsigned short maxvol;
    char minvel;
};

struct note
{
    struct track *track;
    char velocity;
    int pitch;
};

struct trackinfo
{
    size_t notes;
    char *name;
};

unsigned int readvarlen(FILE *f)
{
    unsigned int value = 1;
    char c;
    if ((value = fgetc(f)) & 0x80)
    {
        value &= 0x7f;
        do
            value = (value << 7) + ((c = fgetc(f)) & 0x7f);
        while (c & 0x80);
    }
    return value;
}

void ignore(FILE *f, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        fgetc(f);
}

int scalevolume(char velocity, unsigned short maxvol)
{
    return velocity / 127.0 * maxvol;
}

void freeevents(struct event *item)
{
    if (item != NULL)
        do
        {
            struct event *temp = item;
            item = item->next;
            free(temp->data);
            free(temp);
        } while (item != NULL);
}

void freetrackconfig(struct track *trackconfig, short ntracks)
{
    for (short i = 0; i < ntracks; ++i)
        if (trackconfig[i].keyword != KEYWORD)
            free(trackconfig[i].keyword);
    free(trackconfig);
}

void freetrackinfo(struct trackinfo info[], short ntracks)
{
    for (short t = 0; t < ntracks; ++t)
        free(info[t].name);
}

void printhelptext(void)
{
    printf(
        "Usage:\n"
        "\n"
        "midi230 [-h] [-i] [-t] [-c <track_config_file>] [-o <output file>] <input_file>\n"
        "\n"
        "-h                          Shows this text.\n"
        "-i                          Show midi file info.\n"
        "-t                          Create template config file.\n"
        "-o <output file>            Output file path. By default, it's the input file name an appropriate extension.\n"
        "-c <track_config_file>      A config file of keywords and other settings for each track.\n"
        "\n"
        "Note: flags cannot be combined.\n");
    exit(EXIT_SUCCESS);
}

struct trackinfo parsetrack(FILE *, struct event *, struct track *, char *);
struct event *getnextevent(FILE *, unsigned long *, char *, char, char *);
char *getfname(char *);

int main(int argc, char **argv)
{
    // Parse arguments
    char *midifname = NULL;
    char *outfname = NULL;
    char *configfname = NULL;
    char showinfo = 0, createtemplate = 0;
    {
        if (argc == 1)
            printhelptext();

        size_t i = 1;
        while (1)
        {
            if (strcmp(argv[i], "-h") == 0)
                printhelptext();
            else if (!showinfo && strcmp(argv[i], "-i") == 0)
                showinfo = 1;
            else if (!createtemplate && strcmp(argv[i], "-t") == 0)
                createtemplate = 1;
            else if (outfname == NULL && strcmp(argv[i], "-o") == 0)
            {
                if (++i == argc)
                {
                    fprintf(stderr, "Missing output file path\n");
                    return EXIT_FAILURE;
                }
                outfname = argv[i];
            }
            else if (configfname == NULL && strcmp(argv[i], "-c") == 0)
            {
                if (++i == argc)
                {
                    fprintf(stderr, "Missing track config file path\n");
                    return EXIT_FAILURE;
                }
                configfname = argv[i];
            }
            else
            {
                if (i < argc - 1)
                {
                    fprintf(stderr, "Too many arguments\n");
                    return EXIT_FAILURE;
                }
                midifname = argv[i];
                break;
            }
            if (++i == argc)
            {
                fprintf(stderr, "Not enough arguments\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Starting dummy track event
    struct event placeholderevent = {0, START, NULL, NULL};
    // Pointer to the first actual item in the event list
    struct event *eventlist = NULL;

    // Ticks per quarter note
    short division;

    short ntracks;
    struct track *trackconfig;

    // Read the input file
    {
        FILE *midi = fopen(midifname, "rb");

        if (midi == NULL)
        {
            const char fmt[] = "Unable to open input file '%s'";
            char err[strlen(midifname) + sizeof(fmt)];
            snprintf(err, sizeof(err), fmt, midifname);
            perror(err);
            return EXIT_FAILURE;
        }

        // Get header info
        short format;
        {
            char magic[5] = {'\0'};
            fread(magic, 1, 4, midi);
            unsigned int size = readbig_ui32(midi);
            if (strcmp(magic, "MThd") != 0 || size != 6)
            {
                fprintf(stderr, "Unknown file type\n");
                fclose(midi);
                return EXIT_FAILURE;
            }

            format = readbig_i16(midi);
            ntracks = readbig_i16(midi);
            division = readbig_i16(midi);

            if (format == 2 || division < 0)
            {
                fprintf(stderr, "Unable to interpret midi files of this format\n");
                fclose(midi);
                return EXIT_FAILURE;
            }
        }

        // Create track configuration
        trackconfig = malloc(sizeof(struct track) * ntracks);

        // Set default values
        for (short i = 0; i < ntracks; ++i)
        {
            trackconfig[i].keyword = KEYWORD;
            trackconfig[i].offset = PITCH_OFFSET;
            trackconfig[i].maxvol = MAX_VOLUME;
            trackconfig[i].minvel = CUTOFF_VEL;
        }

        // Read config file
        if (configfname)
        {
            FILE *conf = fopen(configfname, "r");

            if (conf == NULL)
            {
                const char fmt[] = "Unable to open config file '%s'";
                char err[strlen(configfname) + sizeof(fmt)];
                snprintf(err, sizeof(err), fmt, configfname);
                perror(err);
                freetrackconfig(trackconfig, ntracks);
                fclose(midi);
                return EXIT_FAILURE;
            }

            short t = 0;
            char line[128];
            while (fgets(line, sizeof(line), conf))
            {
                char *s = line;
                while (*s == ' ' || *s == '\t' || *s == '\r')
                    ++s;

                if (*s == '#' || *s == '\n')
                    continue;

                char *tokens[4];
                for (int i = 0; i < 4; ++i)
                {
                    tokens[i] = strtok(i ? NULL : s, ", \t\r\n");
                    if (tokens[i] == NULL)
                    {
                        fprintf(stderr, "Syntax error in config file\n");
                        fclose(conf);
                        freetrackconfig(trackconfig, ntracks);
                        fclose(midi);
                        return EXIT_FAILURE;
                    }
                }
                char *keyword = malloc(strlen(tokens[0]) + 1);
                strcpy(keyword, tokens[0]);
                trackconfig[t].keyword = keyword;
                trackconfig[t].offset = (int)strtol(tokens[1], NULL, 10);
                trackconfig[t].maxvol = (unsigned short)strtol(tokens[2], NULL, 10);
                trackconfig[t].minvel = (char)strtol(tokens[3], NULL, 10);
                if (++t == ntracks)
                    break;
            }

            fclose(conf);
        }

        // Get and merge tracks
        struct trackinfo info[ntracks];
        {
            short t = 0;
            while (t < ntracks)
            {
                char chunktype[5] = {'\0'};
                fread(chunktype, 1, 4, midi);
                unsigned int size = readbig_ui32(midi);
                char error = 0;
                // Parse if midi track chunk is found
                if (strcmp(chunktype, "MTrk") == 0)
                {
                    info[t++] = parsetrack(midi, &placeholderevent, trackconfig + t, &error);
                    if (error)
                    {
                        fprintf(stderr, "Input file is corrupted\n");
                        freeevents(eventlist);
                        freetrackconfig(trackconfig, ntracks);
                        freetrackinfo(info, t);
                        fclose(midi);
                        return EXIT_FAILURE;
                    }
                }
                // Ignore unknown chunks
                else
                    ignore(midi, size);
            }
        }
        eventlist = placeholderevent.next;

        // Done reading
        fclose(midi);

        // Show midi info
        if (showinfo)
        {
            printf("MIDI File Info:\n\n");
            printf("Format: %d\n", format);
            printf("Number of tracks: %d\n", ntracks);
            printf("Ticks/quarter note: %d\n\n", division);
            printf("Track   Notes   Name\n");
            for (short t = 0; t < ntracks; ++t)
                printf("%5d %7lu   %s\n", t + 1, info[t].notes, info[t].name ? info[t].name : "");

            for (short t = 0; t < ntracks; ++t)
                free(info[t].name);
            freeevents(eventlist);
            freetrackconfig(trackconfig, ntracks);
            freetrackinfo(info, ntracks);
            return EXIT_SUCCESS;
        }

        // Create template
        if (createtemplate)
        {
            FILE *template;

            if (outfname == NULL)
            {
                char temp[strlen(midifname) + 1];
                strcpy(temp, midifname);
                char *fname = getfname(temp);
                const char ext[] = "%s.conf";
                char path[strlen(fname) + sizeof(ext)];
                snprintf(path, sizeof(path), ext, fname);
                if ((template = fopen(path, "w+")) == NULL)
                {
                    const char fmt[] = "Unable to create template file '%s'";
                    char err[sizeof(path) + sizeof(fmt)];
                    snprintf(err, sizeof(err), fmt, path);
                    perror(err);
                    freeevents(eventlist);
                    freetrackconfig(trackconfig, ntracks);
                    freetrackinfo(info, ntracks);
                    return EXIT_FAILURE;
                }
            }
            else if ((template = fopen(outfname, "w+")) == NULL)
            {
                const char fmt[] = "Unable to create template file '%s'";
                char err[strlen(outfname) + sizeof(fmt)];
                snprintf(err, sizeof(err), fmt, outfname);
                perror(err);
                freeevents(eventlist);
                freetrackconfig(trackconfig, ntracks);
                freetrackinfo(info, ntracks);
                return EXIT_FAILURE;
            }

            fprintf(template, "# Available Tracks:\n\n");
            fprintf(template, "# Track   Notes   Name\n");
            for (short t = 0; t < ntracks; ++t)
                fprintf(template, "# %5d %7lu   %s\n", t + 1, info[t].notes, info[t].name ? info[t].name : "");

            fprintf(template, "\n"
                              "# Each line corresponds to each available track in order.\n"
                              "# ! Every line must be fully filled to prevent unexpected behavior !\n"
                              "# ! i.e. There must be something before and after every comma !\n"
                              "# ! The last line must end with a newline character !\n"
                              "\n"
                              "# Keyword, Base pitch, Max volume, Cutoff Velocity\n");

            for (short t = 0; t < ntracks; ++t)
                fprintf(template, "%s, %d, %d, %d\n", KEYWORD, PITCH_OFFSET, MAX_VOLUME, CUTOFF_VEL);

            freeevents(eventlist);
            freetrackconfig(trackconfig, ntracks);
            freetrackinfo(info, ntracks);
            fclose(template);
            return EXIT_SUCCESS;
        }

        freetrackinfo(info, ntracks);
    }

    // Convert absolute ticks into relative ticks
    {
        unsigned long currenttick = 0;
        struct event *trackhead = eventlist;
        if (trackhead != NULL)
            do
            {
                unsigned long temp = trackhead->tick;
                trackhead->tick -= currenttick;
                currenttick = temp;
                trackhead = trackhead->next;
            } while (trackhead != NULL);
    }

    // Create the output file
    {
        FILE *out;

        if (outfname == NULL)
        {
            char temp[strlen(midifname) + 1];
            strcpy(temp, midifname);
            char *fname = getfname(temp);
            const char ext[] = "%s.moai";
            char path[strlen(fname) + sizeof(ext)];
            snprintf(path, sizeof(path), ext, fname);
            if ((out = fopen(path, "w+")) == NULL)
            {
                const char fmt[] = "Unable to create output file '%s'";
                char err[sizeof(path) + sizeof(fmt)];
                snprintf(err, sizeof(err), fmt, path);
                perror(err);
                freeevents(eventlist);
                freetrackconfig(trackconfig, ntracks);
                return EXIT_FAILURE;
            }
        }
        else if ((out = fopen(outfname, "w+")) == NULL)
        {
            const char fmt[] = "Unable to create output file '%s'";
            char err[strlen(outfname) + sizeof(fmt)];
            snprintf(err, sizeof(err), fmt, outfname);
            perror(err);
            freeevents(eventlist);
            freetrackconfig(trackconfig, ntracks);
            return EXIT_FAILURE;
        }

        // Microseconds per midi quarter note
        unsigned int microsperquarter = 5e5;
        double lastmicrosdelta = 0;
        int lastvolume = -1;
        struct event *trackhead = eventlist;
        while (1)
        {
            if (trackhead->type == NOTE_ON)
            {
                // Find the event where there's a delay until the following event or last element is reached.
                struct event *end = trackhead;
                double microsdelta;
                double bpm;
                while (end->next != NULL)
                {
                    // Next event delay is non-zero.
                    if (end->next->tick > 0)
                    {
                        double microspertick = (double)microsperquarter / (double)division;
                        microsdelta = microspertick * end->next->tick;
                        bpm = 6e7 / microsdelta;
                        if (bpm > 10000)
                        {
                            // If delay is negligable, keep advancing the end pointer.
                            end = end->next;
                            // Change the tempo if the event right after the negligable delay is a tempo change.
                            if (end->type == TEMPO)
                                microsperquarter = *(unsigned int *)end->data;
                        }
                        else
                            // Otherwise, stop and break out.
                            break;
                    }
                    else
                        end = end->next;
                }

                // If the last element wasn't reached, write the speed if it's different from the last one.
                if (end->next != NULL && microsdelta != lastmicrosdelta)
                {
                    lastmicrosdelta = microsdelta;
                    fprintf(out, "!speed@%.3f|", bpm);
                }

                // Write at least one note.
                struct note *notedata = trackhead->data;
                int volume;
                if ((volume = scalevolume(notedata->velocity, notedata->track->maxvol)) != lastvolume)
                {
                    lastvolume = volume;
                    fprintf(out, "!volume@%d|", volume);
                }
                fprintf(out, "%s@%d|", notedata->track->keyword, notedata->pitch - notedata->track->offset);

                // Write more notes until the end pointer is reached
                while (trackhead != end)
                {
                    trackhead = trackhead->next;
                    notedata = trackhead->data;
                    if (trackhead->type == NOTE_ON)
                    {
                        fprintf(out, "!combine|");
                        if ((volume = scalevolume(notedata->velocity, notedata->track->maxvol)) != lastvolume)
                        {
                            lastvolume = volume;
                            fprintf(out, "!volume@%d|", volume);
                        }
                        fprintf(out, "%s@%d|", notedata->track->keyword, notedata->pitch - notedata->track->offset);
                    }
                }

                // If the last element was reached break out of the loop.
                if (end->next == NULL)
                    break;
            }
            else if (trackhead->type == TEMPO)
            {
                // If there's no next event, there's nothing left to do.
                if (trackhead->next == NULL)
                    break;

                microsperquarter = *(unsigned int *)trackhead->data;

                // Check if there's a delay until the next event
                if (trackhead->next->tick > 0)
                {
                    double microspertick = (double)microsperquarter / (double)division;
                    lastmicrosdelta = microspertick * trackhead->next->tick;
                    double bpm = 6e7 / lastmicrosdelta;

                    // Ignore that delay if it is negligable
                    if (bpm <= 10000)
                        fprintf(out, "!speed@%.4f|_pause|", bpm);
                }
            }
            trackhead = trackhead->next;
        }

        fclose(out);
    }

    freeevents(eventlist);
    freetrackconfig(trackconfig, ntracks);

    return 0;
}

// Parses a track and merges it
struct trackinfo parsetrack(FILE *stream, struct event *trackhead, struct track *trackconfig, char *error)
{
    unsigned long currenttick = 0;
    unsigned char runningstatus = 0;
    struct trackinfo info = {0, NULL};

    struct event *newevent;
    while ((newevent = getnextevent(stream, &currenttick, &runningstatus, trackconfig->minvel, error)) != NULL)
    {
        if (newevent->type == TRACK_NAME)
        {
            if (info.name == NULL)
                info.name = (char *)newevent->data;
            free(newevent);
            continue;
        }

        if (newevent->type == NOTE_ON)
        {
            ++info.notes;
            ((struct note *)newevent->data)->track = trackconfig;
        }

        // Advance the track head until the end is reached or the next event absolute tick is greater than the new event
        while (trackhead->next != NULL && trackhead->next->tick <= newevent->tick)
            trackhead = trackhead->next;

        if (trackhead->next == NULL)
            // Append the new event
            newevent->next = NULL;
        else
            // Insert the new event
            newevent->next = trackhead->next;
        trackhead->next = newevent;
        trackhead = newevent;
    }

    if (info.name == NULL)
    {
        info.name = malloc(1);
        *info.name = '\0';
    }

    return info;
}

// Gets the next midi/sysex/meta event
struct event *getnextevent(FILE *stream, unsigned long *currenttick, char *runningstatus, char minvel, char *error)
{
    while (1)
    {
        *currenttick += readvarlen(stream);

        unsigned char status = fgetc(stream);
        switch (status)
        {
        // midi event
        default:
        {
            // New status
            if (status & 0x80)
                *runningstatus = status;
            // There's no new status, rewind stream by one byte.
            else
                fseek(stream, -1, SEEK_CUR);

            unsigned char highnybble = *runningstatus & 0xF0;
            unsigned char lownybble = *runningstatus & 0x0F;

            switch (highnybble)
            {
            // Create note on event
            case 0x90:
            {
                int pitch = fgetc(stream);
                char velocity = fgetc(stream);
                // Only create a note if it is audable
                if (velocity > minvel)
                {
                    struct event *newevent = malloc(sizeof(struct event));

                    newevent->type = NOTE_ON;
                    newevent->tick = *currenttick;

                    struct note *notedata = malloc(sizeof(struct note));
                    notedata->pitch = pitch;
                    notedata->velocity = velocity;
                    newevent->data = notedata;

                    return newevent;
                }
            }
            break;

            // Ignore other events
            case 0x80:
            case 0xA0:
            case 0xB0:
            case 0xE0:
                fgetc(stream);
            case 0xC0:
            case 0xD0:
                fgetc(stream);
                break;

            // System common messages
            case 0xF0:
            {
                switch (lownybble)
                {
                case 0b0000:
                    while (fgetc(stream) != 0b11110111)
                        ;
                    break;
                // Ignore 1 or 2 bytes
                case 0b0010:
                    fgetc(stream);
                case 0b0011:
                    fgetc(stream);
                }
            }
            break;

            default:
            {
                *error = 1;
                return NULL;
            }
            }
        }
        break;

        // sysex event
        case 0xF0:
        case 0xF7:
        {
            unsigned int length = readvarlen(stream);
            ignore(stream, length);

            // Clear running status
            *runningstatus = 0;
        }
        break;

        // meta event
        case 0xFF:
        {
            char type = fgetc(stream);
            unsigned int length = readvarlen(stream);

            switch (type)
            {
            // End of track
            case 0x2F:
                return NULL;

            // Create tempo event
            case 0x51:
            {
                struct event *newevent = malloc(sizeof(struct event));

                newevent->type = TEMPO;
                newevent->tick = *currenttick;

                unsigned int *mspt = malloc(sizeof(int));
                for (int i = 0; i < 3; ++i)
                    *mspt = (*mspt << 8) | fgetc(stream);
                newevent->data = mspt;

                return newevent;
            }

            // Get track name
            case 0x03:
            {
                struct event *newevent = malloc(sizeof(struct event));

                newevent->type = TRACK_NAME;

                char *name = malloc(length + 1);
                for (int i = 0; i < length; ++i)
                    name[i] = fgetc(stream);
                name[length] = '\0';
                newevent->data = name;

                return newevent;
            }

            // Ignore other meta events
            default:
                ignore(stream, length);
            }

            // Clear running status
            *runningstatus = 0;
        }
        }
    }
}

// Gets the file name from a path without the extension
char *getfname(char *path)
{
    char *preserve = path;
    char *next = strtok(path, "/\\");
    if (next != NULL)
    {
        do
        {
            preserve = next;
            next = strtok(NULL, "/\\");
        } while (next != NULL);
    }

    char *n = NULL;
    for (char *c = preserve + 1; *c != '\0'; ++c)
        if (*c == '.')
            n = c;
    if (n)
        *n = '\0';

    return preserve;
}