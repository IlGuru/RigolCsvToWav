#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>

#define FLT_MAX		__FLT_MAX__
#define NUM_CANALI  4
#define MAX_COLONNE NUM_CANALI+2

//	csv file
typedef struct {
	FILE*	fd;
	char*	nome_file;
	int		num_colonne;
	char	colonna[MAX_COLONNE][32];
	char	s[256];
} s_csv_file;

typedef struct {
	float	f_min;
	float	f_max;
	float	f_cm;
	float	f_amp;
} s_stat;

typedef struct {
	int		num_canali;
	float	f_dato[NUM_CANALI];
	float	f_dato_norm[NUM_CANALI];
	s_stat	stat[NUM_CANALI];
} s_dati;

typedef struct {
   unsigned int f_file_in:1;
   unsigned int f_file_out:1;
   unsigned int f_sample_rate:1;
   unsigned int f_skip_rows:1;
   unsigned int f_verbose:1;
   unsigned int f_very_verbose:1;
} s_status_flags;

typedef union u_values {
	char*	s;
	int		i;
} u_value;

typedef struct {
	u_value val;
	int   	pos;
} s_parametro;

typedef struct {
	s_parametro		file_in;
	s_parametro 	file_out;
	s_parametro 	sample_rate;
	s_parametro 	skip_rows;
	s_parametro 	verbose;
	s_parametro 	vverbose;
	s_status_flags	flags;
} s_param;

//	wav file
typedef struct {
	SNDFILE*	fd;
	SF_INFO 	sfinfo;
	char*		nome_file;
} s_wav_file;

void InitCsv( s_csv_file* p_csv )
{
	int			i_ind;
	
	p_csv->num_colonne 	= 0;
	for ( i_ind=0; i_ind<MAX_COLONNE; i_ind++ )
	{
		p_csv->colonna[ i_ind ][0] = '\0';
	}

}

void LeggiCsv( s_csv_file* p_csv )
{
	char* 	token;
	int		i_ind	= 0;
	int		i_num	= 0;
	
	fscanf( p_csv->fd, "%s", p_csv->s );
	
	//	Conto il numero di colonne nella stringa (minimo 1 colonna)
	i_num = 1;
	for( i_ind=0; p_csv->s[i_ind]!='\0'; i_ind++ )
		if ( p_csv->s[i_ind] == ',' ) i_num++;
	p_csv->num_colonne 	= i_num;
	
	//	Leggo i token
	token = strtok( p_csv->s, "," );
	i_ind = 0;
	while( token != NULL )
	{
		strcpy( p_csv->colonna[ i_ind ], token );
		i_ind++;
		
		token = strtok( NULL, "," );
	}
	
}

void PrintCsv( s_csv_file* p_csv )
{
	int			i_ind;

	for ( i_ind=0; i_ind<p_csv->num_colonne; i_ind++ )
		printf( "%s ", p_csv->colonna[ i_ind ] );
	printf( "\n" );
}

void InitDati( s_dati* p_dati )
{
	int			i_ind;

	p_dati->num_canali	= 0;
	for ( i_ind=0; i_ind<NUM_CANALI; i_ind++ )
	{
		p_dati->f_dato[ i_ind ]		= 0.0;
		p_dati->stat[ i_ind ].f_max =-FLT_MAX;
		p_dati->stat[ i_ind ].f_min = FLT_MAX;
		p_dati->stat[ i_ind ].f_cm	= 0.0;
		p_dati->stat[ i_ind ].f_amp	= 0.0;
		p_dati->f_dato_norm[ i_ind ]= 0.0;
	}
}

void PrintDati( s_dati* p_dati )
{
	int			i_ind;

	for ( i_ind=0; i_ind<p_dati->num_canali; i_ind++ )
		printf( "%g ", p_dati->f_dato[ i_ind ] );
	printf( "\n" );
}

void ConvertiDati( s_csv_file* p_csv, s_dati* p_dati )
{
	int			i_ind;
	
	p_dati->num_canali = p_csv->num_colonne;
	
	for ( i_ind=0; i_ind < NUM_CANALI; i_ind++ )
	{
		if ( i_ind < p_csv->num_colonne )
		{
			sscanf( p_csv->colonna[i_ind], "%g", &p_dati->f_dato[i_ind] );
			if ( p_dati->f_dato[i_ind] < p_dati->stat[i_ind].f_min )
				p_dati->stat[i_ind].f_min = p_dati->f_dato[i_ind];
			if ( p_dati->f_dato[i_ind] > p_dati->stat[i_ind].f_max )
				p_dati->stat[i_ind].f_max = p_dati->f_dato[i_ind];
		} else {
			p_dati->f_dato[i_ind] = 0.0;
		}
		
	}
}

void CalcolaCmAmp( s_dati* p_dati )
{
	int			i_ind;
	
 	for ( i_ind=0; i_ind < NUM_CANALI; i_ind++ )
	{
		p_dati->stat[i_ind].f_cm = ( p_dati->stat[i_ind].f_max + p_dati->stat[i_ind].f_min ) / 2.0;
		p_dati->stat[i_ind].f_amp = ( p_dati->stat[i_ind].f_max - p_dati->stat[i_ind].f_min ) / 2.0;
	}
}

void PrintStatsDati( s_dati* p_dati )
{
	int			i_ind;
	
	printf( "   Statistiche:\n" );
 	for ( i_ind=0; i_ind < p_dati->num_canali; i_ind++ )
	{
		printf( "      Canele %d:\n", i_ind );
		printf( "         Valore min:     %g\n", p_dati->stat[i_ind].f_min );
		printf( "         Valore max:     %g\n", p_dati->stat[i_ind].f_max );
		printf( "         Valore mediano: %g\n", p_dati->stat[i_ind].f_cm );
		printf( "         Valore ampezza: %g\n", p_dati->stat[i_ind].f_amp );
	}
}

void NormalizzaInt( s_dati* p_dati )
{
	int	i_ind;
	
 	for ( i_ind=0; i_ind < NUM_CANALI; i_ind++ )
	{
		p_dati->f_dato_norm[i_ind]	= ( p_dati->f_dato[i_ind] - p_dati->stat[i_ind].f_cm ) / p_dati->stat[i_ind].f_amp;
	}
}

void Istruzioni( int argc, char** argv )
{
	int		i_ind;

	printf( "\nIstruzioni:\n" );
	printf( "  RigolCsvToWav.exe [-v] [-vv] -sr sample_rate -i file_in [-o out[.wav]]\n" );
	printf( "    La posizione dei parametri non conta.\n" );
	printf( "    Parametri obbligatori:\n" );
	printf( "      -i : Nome file csv.\n" );
	printf( "      -sr: Sample rate. Parametro obbligatorio.\n" );
	printf( "           Il valore deve essere nella forma N[M]\n");
	printf( "             N: un intero maggiore di zero\n");
	printf( "             M: una stringa case insensitive opzionale del tipo k[*]|m[*]|g[*]\n");
	printf( "                k, K: *10^3\n" );
	printf( "                m, M: *10^6\n" );
	printf( "                g, G: *10^9\n" );
	printf( "                Ogni altra stringa di caratteri e' ininfluente.\n" );
	printf( "    Parametri opzionali:\n" );
	printf( "      -o : Nome file wav.\n" );
	printf( "           Se non specificato viene preso il nome del file in ingresso con estensione '.wav'.\n" );
	printf( "      -sk: Numero righe da ignorare all'inizio del file.\n" );
	printf( "           Di default sono 2.\n" );
	printf( "      -v:  Verbose.\n" );
	printf( "           Visualizza i passaggi.\n" );
	printf( "      -vv: Very Verbose.\n" );
	printf( "           Visualizza i dati letti dal file csv ed alcune statistiche.\n" );
	printf( "\n Es: RigolCsvToWav [-v] -sr 125Mega [-o out[.wav]] file_in.csv\n\n" );
	printf( "Avevi scritto:\n" );
	for ( i_ind=0; i_ind < argc; i_ind++ )
		printf( "%s ", argv[i_ind] );
	printf( "\n" );	
}

int ParseParam( int argc, char** argv, s_param* p_param )
{
	int		i_ind;
	char*	s;
	
	p_param->flags.f_file_in		= 0;
	p_param->flags.f_file_out		= 0;
	p_param->flags.f_sample_rate 	= 0;
	p_param->flags.f_verbose		= 0;
	p_param->flags.f_very_verbose	= 0;
	for ( i_ind=1; i_ind<argc; i_ind++ )
	{
		if ( strcmp( argv[i_ind], "-i" ) == 0 )
		{
			p_param->file_in.pos 			= i_ind+1;
			p_param->flags.f_file_in 		= 1;
			i_ind++;
		}
		if ( strcmp( argv[i_ind], "-sr" ) == 0 )
		{
			p_param->sample_rate.pos 		= i_ind+1;
			p_param->flags.f_sample_rate	= 1;
			i_ind++;
		}
		if ( strcmp( argv[i_ind], "-o" ) == 0 )
		{
			p_param->file_out.pos 			= i_ind+1;
			p_param->flags.f_file_out 		= 1;
			i_ind++;
		}
		if ( strcmp( argv[i_ind], "-sk" ) == 0 )
		{
			p_param->skip_rows.pos 			= i_ind+1;
			p_param->flags.f_skip_rows 		= 1;
			i_ind++;
		}
		if ( strcmp( argv[i_ind], "-v" ) == 0 )
		{
			p_param->verbose.pos			= i_ind;
			p_param->flags.f_verbose 		= 1;
		}
		if ( strcmp( argv[i_ind], "-vv" ) == 0 )
		{
			p_param->vverbose.pos			= i_ind;
			p_param->flags.f_verbose 		= 1;
			p_param->flags.f_very_verbose 	= 1;
		}
	}
	
	//	Leggo il parametro file_in
	if ( p_param->flags.f_file_in )
	{
		p_param->file_in.val.s = (char*)malloc( strlen( argv[p_param->file_in.pos] ) );
		strcpy( p_param->file_in.val.s, argv[p_param->file_in.pos] );
	}
	else
	{
		return 0;
	}
	
	//	Leggo il parametro sample_rate
	if ( p_param->flags.f_sample_rate )
	{
		p_param->sample_rate.val.i = 0;
		s = (char*)malloc( strlen(argv[ p_param->sample_rate.pos ]) );
		i_ind = sscanf( argv[ p_param->sample_rate.pos ], "%d%s", &p_param->sample_rate.val.i, s );
		if ( p_param->sample_rate.val.i <= 0 )
		{
			return 0;
		}
		if ( i_ind == 2 )
		{
			switch ( s[0] | 32 )
			{
				case 'k':
					p_param->sample_rate.val.i *= 1000;
					break;
				case 'm':
					p_param->sample_rate.val.i *= 1000000;
					break;
				case 'g':
					p_param->sample_rate.val.i *= 1000000000;
					break;
				default:
					printf( "   \nWARNING: Non ho riconosciuto il moltiplicatore %s.\n         sample_rate rimane impostato a %d.\n", s, p_param->sample_rate.val.i);
					break;
			}
		}
	}
	else
	{
		return 0;
	}

	//	Leggo numero righe da ignorare all'inizio del file
	if ( p_param->flags.f_skip_rows )
	{
		//	Se è stato passato come parametro leggo il valore dichiarato
		p_param->skip_rows.val.i = 0;
		i_ind = sscanf( argv[ p_param->skip_rows.pos ], "%d", &p_param->skip_rows.val.i );
		if ( (p_param->skip_rows.val.i <= 0) )
		{
			return 0;
		}
	}
	else
	{
		//	Se è stato passato come parametro uso 2 come default
		p_param->skip_rows.pos		=-1;
		p_param->skip_rows.val.i 	= 2;
	}
	
	//	Leggo il parametro opzionale file_out
	if ( p_param->flags.f_file_out )
	{
		//	Se è stato passato come parametro uso il valore dichiarato, aggiungendo l'estensione .wav
		p_param->file_out.val.s = (char*)malloc( strlen( argv[p_param->file_out.pos] ) );
		strcpy( p_param->file_out.val.s, argv[p_param->file_out.pos] );
		p_param->file_out.val.s = strtok( p_param->file_out.val.s, ".wav" );
		strcat( p_param->file_out.val.s, ".wav" );
	}
	else
	{
		//	Se non è stato passato come parametro uso il valore file in a cui sostituisco l'estensione .csv con .wav
		p_param->file_out.pos 	= -1;
		p_param->file_out.val.s = (char*)malloc( strlen( p_param->file_in.val.s ) );
		strcpy( p_param->file_out.val.s, p_param->file_in.val.s );
		p_param->file_out.val.s = strtok( p_param->file_out.val.s, ".csv" );
		strcat( p_param->file_out.val.s, ".wav" );
	}

	return 1;
}

int main(int argc, char **argv)
{
	int			i_ind;
	s_param		Parametri;
	s_csv_file	Csv;
	s_dati		Dati;
	s_wav_file	wav_file;

	//	Validazione parametri input
	if ( !ParseParam( argc, argv, &Parametri ) )
	{
		Istruzioni( argc, argv );
		return 1;
	}
	
	InitCsv( &Csv );
	InitDati( &Dati );

	Csv.nome_file = (char*)malloc( strlen( Parametri.file_in.val.s ) );
	strcpy( Csv.nome_file, Parametri.file_in.val.s );

	wav_file.nome_file = (char*)malloc( strlen( Parametri.file_out.val.s ) );
	strcpy( wav_file.nome_file, Parametri.file_out.val.s );

	if ((Csv.fd = fopen64( Csv.nome_file, "r" )) == NULL )
	{
		printf( "\nErrore apertura file csv.\n" );
		return 1;
	}

	if ( Parametri.flags.f_verbose ) {
		printf( "   Leggo il file per calcolare i valori da normalizzare.\n");
		fflush(stdout);
	}
		
	//	Skip prime righe
	for ( i_ind = 0; i_ind < Parametri.skip_rows.val.i; i_ind++ )
	{
		LeggiCsv( &Csv );
		if ( Parametri.flags.f_very_verbose )
			PrintCsv( &Csv );
	}

	//	Leggo i valori
	while ( !feof( Csv.fd ) )
	{
		LeggiCsv( &Csv );
		if ( Parametri.flags.f_very_verbose )
			PrintCsv( &Csv );
		ConvertiDati( &Csv, &Dati );
	}

	//	Statistiche
	CalcolaCmAmp( &Dati );
	if ( Parametri.flags.f_verbose ) {
		PrintStatsDati( &Dati );
		fflush(stdout);
	}
	
	//	-------------------------------------------------------------
	
	//	Rileggo il file
	rewind( Csv.fd );
	
	//	Formato file wav
	wav_file.sfinfo.format		= SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	wav_file.sfinfo.channels	= Dati.num_canali;
	wav_file.sfinfo.samplerate	= Parametri.sample_rate.val.i;
	
	//	Apertura file wav in scrittura
	if ((wav_file.fd = sf_open(wav_file.nome_file, SFM_WRITE, &wav_file.sfinfo)) == NULL )
	{
		printf( "\nErrore apertura file wav.\n" );
		return 1;
	}

	//	Skip prime righe
	for ( i_ind = 0; i_ind < Parametri.skip_rows.val.i; i_ind++ )
	{
		LeggiCsv( &Csv );
	}

	if ( Parametri.flags.f_verbose ) {
		printf( "   Scrivo file wav.\n");
		fflush(stdout);
	}
		
	while ( !feof( Csv.fd ) )
	{
		LeggiCsv( &Csv );
		ConvertiDati( &Csv, &Dati );
		NormalizzaInt( &Dati );

		sf_writef_float( wav_file.fd, Dati.f_dato_norm, 1);
	}
	
	//	Chiusura files
	if ( Parametri.flags.f_verbose ) {
		printf( "   Ho finito.\n");
		fflush(stdout);
	}
		
	if ( Csv.fd != NULL )
		fclose( Csv.fd );
	
	if ( wav_file.fd != NULL )
		sf_close( wav_file.fd );
		
	//	Fine
	return 0;
}
