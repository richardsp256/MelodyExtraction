// the following is included so we can handle the basics of midi files

struct Track{
  unsigned char* data;
  int len;
};

struct Midi{
  struct Track** tracks;
  int format; //0 = single track, 1 = multitrack, 2 = multisong.
  int numTracks; //number of tracks
  int division; //delts time. if positive, represents units per beat. if negative, in SMPTE compatible units. 
};

struct Track* GenerateTrack(int* noteArr, int size, int verbose);
struct Midi* GenerateMIDI(int* noteArr, int size, int verbose);
void freeMidi(struct Midi* midi);
void SaveMIDI(struct Midi* midi, char* path, int verbose);


// like libfvad, libsamplerate, and fftw3, we define an object that we
// use to actually execute the library
// we may want to make this opaque

struct me_data;

// create an instance of me_data
struct me_data* me_data_new();

// destroy me_data
void me_data_free(struct me_data *inst);

// reset me_data to default values
int me_data_reset(struct me_data *inst);

/* List of adjustable parameters:
 * pitch_window: number of frames of audiodata taken for each stft window for 
 *                pitch detection. def = 4096
 * etc.
 *
 * below are functions to adjust algorithm parameters
 * each of these functions return 0, 1, or 2.
 * 0 means that adjusting the parameter was successful
 * 1 indicates that there was an issue with allocating memory
 * 2 means that the parameter was invalid
 */


/* pitch_window: number of frames of audiodata taken for each stft window for 
 *                pitch detection. def = 4096
 */
int me_set_prefix(struct me_data* inst,char* value);
int me_set_pitch_window(struct me_data* inst,char* value);
int me_set_pitch_padded(struct me_data* inst,char* value);
int me_set_pitch_spacing(struct me_data* inst,char* value);
int me_set_pitch_strategy(struct me_data* inst,char* value);
int me_set_onset_window(struct me_data* inst,char* value);
int me_set_onset_padded(struct me_data* inst,char* value);
int me_set_onset_spacing(struct me_data* inst,char* value);
int me_set_onset_strategy(struct me_data* inst,char* value);
int me_set_silence_window(struct me_data* inst, char* value);
int me_set_silence_spacing(struct me_data* inst, char* value);
int me_set_silence_strategy(struct me_data* inst, char* value);
int me_set_silence_mode(struct me_data* inst,int value);
int me_set_hps_overtones(struct me_data* inst,int value);
int me_set_tuning(struct me_data* inst,int value);
int me_set_verbose(struct me_data* inst,int value);


struct Midi* me_process(char *fname, struct me_data *inst);
