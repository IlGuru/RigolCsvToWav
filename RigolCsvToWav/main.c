/*
	Copyright 2015 Stefano Busnelli
		Verion: 		1.3.0
		Last modified:	2015-11-24

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published 
	by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define	USE_SNDFILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#ifdef USE_SNDFILE
#include <sndfile.h>
#endif

#define S_VERSION "1.3.0"

#define LEN_ROW 256
#define LEN_COL 32

//	csv file
typedef struct {
	FILE*	fd;
	char*	nome_file;
	int		num_colonne;
	char**	colonna;
	char*	s;				//	Stringa ledda dal file csv
	char*	c;				//	Copia di s su cui lavorare
} s_csv_file;

//	stats
typedef struct {
	float	f_min;
	float	f_max;
	float	f_cm;
	float	f_amp;
	int		num_dati;
} s_stat;

//	dati
typedef struct {
	int		num_canali;
	float*	f_dato;
	short*	f_dato_norm;
	s_stat*	stat;
	int		num_dati;
} s_dati;

//	flag parametri
typedef struct {
   unsigned int f_file_in:1;
   unsigned int f_file_out:1;
   unsigned int f_sample_rate:1;
   unsigned int f_skip_rows:1;
   unsigned int f_verbose:1;
   unsigned int f_very_verbose:1;
   unsigned int f_help:1;
} s_status_flags;

//	valore parametro
typedef union u_values {
	char*	s;
	int		i;
} u_value;

//	parametro
typedef struct {
	u_value val;
	int   	pos;
} s_parametro;

//	parametri
typedef struct {
	s_parametro		file_in;
	s_parametro 	file_out;
	s_parametro 	sample_rate;
	s_parametro 	skip_rows;
	s_parametro 	verbose;
	s_parametro 	vverbose;
	s_parametro 	help;
	s_status_flags	flags;
} s_param;

#ifndef USE_SNDFILE

//	---------------------------------------------
//	Formato RIFF file wav

typedef struct {
	char	ChunkID[4];
	int		ChunkSize;
	char	Format[4];
} s_riff_cd;

typedef struct {
	char	Subchunk1ID[4];
	int		Subchunk1Size;
	short	AudioFormat;
	short	NumChannels;
	int		SampleRate;
	int		ByteRate;
	short	BlockAlign;
	short	BitsPerSample;
} s_fmt_sc;

typedef struct {
	char	Subchunk2ID[4];
	int		Subchunk2Size;
} s_data_sc;

typedef struct {
	s_riff_cd	riff_cd;
	s_fmt_sc	fmt_sc;
	s_data_sc	data_sc;
	short*		Data;
	short*		pData;
} s_wav_riff_hd;

#endif

//	---------------------------------------------

//	wav file
typedef struct {
	char*			nome_file;
#ifndef USE_SNDFILE
	FILE*			fd;
	s_wav_riff_hd*	wh;
#else
	SNDFILE*	fd;
	SF_INFO 	sfinfo;	
#endif
} s_wav_file;

void InitCsv( s_csv_file* p_csv )
{
	p_csv->s 			= (char*)calloc( 1, LEN_ROW + 1 );
	p_csv->s[0] 		= '\0';

	p_csv->num_colonne 	= 0;
	p_csv->colonna		= NULL;
	p_csv->c			= NULL;
}

void InitColonneCsv( s_csv_file* p_csv, int i_num_colonne )
{
	int			i_ind;

	if ( p_csv->num_colonne != i_num_colonne && p_csv->colonna != NULL )
	{
		for ( i_ind=0; i_ind<p_csv->num_colonne; i_ind++ )
		{
			if ( p_csv->colonna[ i_ind ] != NULL )
				free( p_csv->colonna[ i_ind ] );
				p_csv->colonna[ i_ind ] = NULL;
		}
		free( p_csv->colonna );
		p_csv->colonna = NULL;
	}
	
	p_csv->num_colonne 	= i_num_colonne;
	p_csv->colonna = (char**) calloc( i_num_colonne, sizeof( char** ) );
	for ( i_ind=0; i_ind<i_num_colonne; i_ind++ )
	{
		p_csv->colonna[ i_ind ] = (char*)calloc( 1, LEN_COL + 1 );
		p_csv->colonna[ i_ind ][0] = '\0';
	}

}

void LeggiCsv( s_csv_file* p_csv )
{

	p_csv->s[0] = '\0';
	if ( fgets( p_csv->s, LEN_ROW, p_csv->fd ) != NULL )
	{
		sscanf( p_csv->s, "%s", p_csv->s );		
		if ( p_csv->s[0] == '\n' )
			p_csv->s[0] = '\0';
	}	

}

int ContaColonneCsv( s_csv_file* p_csv )
{
	int		i_ind	= 0;
	int		i_num	= 0;

	//	Conto il numero di colonne nella stringa (minimo 1 colonna)
	i_num = 0;
	for( i_ind=0; p_csv->s[i_ind]!='\0'; i_ind++ )
		if ( p_csv->s[i_ind] == ',' ) i_num++;

	return i_num;

}

int TokenizzaCsv( s_csv_file* p_csv )
{
	char* 	token		= NULL;
	int		i_ind		= 0;
	int		i_num_token	= 0;

	p_csv->c = (char*)calloc( 1, strlen( p_csv->s ) + 1 );
	strcpy( p_csv->c, p_csv->s );
	
	//	Leggo i token
	token = strtok( p_csv->c, "," );
	i_ind = 0;
	while( token != NULL )
	{
		if ( i_ind < p_csv->num_colonne )
			strcpy( p_csv->colonna[ i_ind ], token );
			
		i_ind++;

		token = strtok( NULL, "," );
	}

	i_num_token = i_ind;
	
	while ( i_ind < p_csv->num_colonne )
	{
		p_csv->colonna[ i_ind ][0] = '\0';
		i_ind++;
	}

	free( p_csv->c );
	p_csv->c = NULL;
	
	return i_num_token;
}

void PrintCsv( s_csv_file* p_csv )
{
	int			i_ind;

	for ( i_ind=0; i_ind<p_csv->num_colonne; i_ind++ )
		printf( "%16s ", p_csv->colonna[ i_ind ] );
}

void InitDati( s_dati* p_dati, int i_num_canali )
{
	int			i_ind;

	p_dati->num_canali	= i_num_canali;

	p_dati->f_dato		= (float*)	calloc( i_num_canali, sizeof( float ) );
	p_dati->f_dato_norm	= (short*)	calloc( i_num_canali, sizeof( short ) );
	p_dati->stat		= (s_stat*)	calloc( i_num_canali, sizeof( s_stat ) );
	p_dati->num_dati	= 0;
	
	for ( i_ind=0; i_ind<i_num_canali; i_ind++ )
	{
		p_dati->stat[ i_ind ].f_max 	=-FLT_MAX;
		p_dati->stat[ i_ind ].f_min 	= FLT_MAX;
	}
}

void PrintDati( s_dati* p_dati )
{
	int			i_ind;

	for ( i_ind=0; i_ind<p_dati->num_canali; i_ind++ )
		printf( "% 10.6f ", p_dati->f_dato[ i_ind ] );
}

void ConvertiDati( s_csv_file* p_csv, s_dati* p_dati )
{
	int			i_ind;
	int         i_num;

	p_dati->num_dati++;
	for ( i_ind=0; i_ind < p_dati->num_canali; i_ind++ )
	{
		i_num = sscanf( p_csv->colonna[i_ind], "%g", &p_dati->f_dato[i_ind] );
		if ( i_num == 1 )
		{
			p_dati->stat[ i_ind ].num_dati++;
			if ( p_dati->f_dato[i_ind] < p_dati->stat[i_ind].f_min )
				p_dati->stat[i_ind].f_min = p_dati->f_dato[i_ind];
			if ( p_dati->f_dato[i_ind] > p_dati->stat[i_ind].f_max )
				p_dati->stat[i_ind].f_max = p_dati->f_dato[i_ind];
		}
		else
		{
			p_dati->f_dato[i_ind] = 0.0;
		}
		
	}

}

void CalcolaCmAmp( s_dati* p_dati )
{
	int			i_ind;
	
 	for ( i_ind=0; i_ind < p_dati->num_canali; i_ind++ )
	{
		if ( p_dati->stat[i_ind].f_max >= p_dati->stat[i_ind].f_min ) {
			p_dati->stat[i_ind].f_cm = ( p_dati->stat[i_ind].f_max + p_dati->stat[i_ind].f_min ) / 2.0;
			p_dati->stat[i_ind].f_amp = ( p_dati->stat[i_ind].f_max - p_dati->stat[i_ind].f_min ) / 2.0;
		} 
		else 
		{
			p_dati->stat[i_ind].f_min 	= 0.0;
			p_dati->stat[i_ind].f_max	= 0.0;
			p_dati->stat[i_ind].f_cm	= 0.0;
			p_dati->stat[i_ind].f_amp	= 0.0;
		}
	}
}

void PrintStatsDati( s_dati* p_dati )
{
	int			i_ind;
	
	printf( "\n   Statistiche:\n" );
 	for ( i_ind=0; i_ind < p_dati->num_canali; i_ind++ )
	{
		printf( "      Canele %d:\n", i_ind );
		printf( "         Valore min:      % 10.6f\n", p_dati->stat[i_ind].f_min );
		printf( "         Valore max:      % 10.6f\n", p_dati->stat[i_ind].f_max );
		printf( "         Valore mediano:  % 10.6f\n", p_dati->stat[i_ind].f_cm );
		printf( "         Valore ampiezza: % 10.6f\n", p_dati->stat[i_ind].f_amp );
		printf( "         Numero dati:     % 10d\n",   p_dati->stat[ i_ind ].num_dati );
	}
	printf( "\n" );

}

void NormalizzaDati( s_dati* p_dati )
{
	int	i_ind;
	
 	for ( i_ind=0; i_ind < p_dati->num_canali; i_ind++ )
	{
		if ( p_dati->stat[i_ind].f_amp == 0 )
		{
			p_dati->f_dato_norm[i_ind]	= 0;
		}
		else
		{
			p_dati->f_dato_norm[i_ind]	= (short) (SHRT_MAX * ( ( ( p_dati->f_dato[i_ind] - p_dati->stat[i_ind].f_cm ) ) / p_dati->stat[i_ind].f_amp ));
		}
	}
}

void Istruzioni( int argc, char** argv )
{
	int		i_ind;

	printf( "\nRigolCsvToWav - Versione %s - Istruzioni:\n", S_VERSION );
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
	printf( "      -?:  Help.\n" );
	printf( "           Visualizza questo menu'.\n" );
	printf( "\n Es: RigolCsvToWav [-v] -sr 125Mega [-o out[.wav]] file_in.csv\n\n" );

	printf( "\nHai scritto:\n" );
	for ( i_ind=0; i_ind < argc; i_ind++ )
		printf( "%s ", argv[i_ind] );
	printf( "\n" );	
}

int ParseParam( int argc, char** argv, s_param* p_param )
{
	int		i_ind;
	char*	s;
	
	p_param->flags.f_file_in		= 0;
	p_param->flags.f_sample_rate 	= 0;
	p_param->flags.f_file_out		= 0;
	p_param->flags.f_skip_rows		= 0;
	p_param->flags.f_verbose		= 0;
	p_param->flags.f_very_verbose	= 0;
	p_param->flags.f_help			= 0;
	for ( i_ind=1; i_ind<argc; i_ind++ )
	{
		if ( i_ind < argc && strcmp( argv[i_ind], "-i" ) == 0 )
		{
			p_param->file_in.pos 			= i_ind+1;
			p_param->flags.f_file_in 		= 1;
			i_ind++;
		}
		if ( i_ind < argc && strcmp( argv[i_ind], "-sr" ) == 0 )
		{
			p_param->sample_rate.pos 		= i_ind+1;
			p_param->flags.f_sample_rate	= 1;
			i_ind++;
		}
		if ( i_ind < argc && strcmp( argv[i_ind], "-o" ) == 0 )
		{
			p_param->file_out.pos 			= i_ind+1;
			p_param->flags.f_file_out 		= 1;
			i_ind++;
		}
		if ( i_ind < argc && strcmp( argv[i_ind], "-sk" ) == 0 )
		{
			p_param->skip_rows.pos 			= i_ind+1;
			p_param->flags.f_skip_rows 		= 1;
			i_ind++;
		}
		if ( i_ind < argc && strcmp( argv[i_ind], "-v" ) == 0 )
		{
			p_param->verbose.pos			= i_ind;
			p_param->flags.f_verbose 		= 1;
		}
		if ( i_ind < argc && strcmp( argv[i_ind], "-vv" ) == 0 )
		{
			p_param->vverbose.pos			= i_ind;
			p_param->flags.f_verbose 		= 1;
			p_param->flags.f_very_verbose 	= 1;
		}
		if ( i_ind < argc && strcmp( argv[i_ind], "-?" ) == 0 )
		{
			p_param->help.pos				= i_ind;
			p_param->flags.f_help 			= 1;
		}
	}
	
	//	Help
	if ( p_param->flags.f_help ) 
	{
		return 0;
	}
	
	//	Leggo il parametro file_in
	if ( p_param->flags.f_file_in )
	{
		if ( p_param->file_in.pos >= argc )
		{
			return 0;
		}
		p_param->file_in.val.s = (char*)calloc( 1, strlen( argv[p_param->file_in.pos] ) + 1 );
		strcpy( p_param->file_in.val.s, argv[p_param->file_in.pos] );
	}
	else
	{
		return 0;
	}
	
	//	Leggo il parametro sample_rate
	if ( p_param->flags.f_sample_rate )
	{
		if ( p_param->sample_rate.pos >= argc )
		{
			return 0;
		}
		p_param->sample_rate.val.i = 0;
		s = (char*)calloc( 1, strlen(argv[ p_param->sample_rate.pos ]) + 1 );
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
		if ( p_param->skip_rows.pos >= argc )
		{
			return 0;
		}
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
		if ( p_param->file_out.pos >= argc )
		{
			return 0;
		}
		//	Se è stato passato come parametro uso il valore dichiarato, aggiungendo l'estensione .wav
		p_param->file_out.val.s = (char*)calloc( 1, strlen( argv[p_param->file_out.pos] + 5 ) );
		strcpy( p_param->file_out.val.s, argv[p_param->file_out.pos] );
		//	Se non trovo l'estensione .wav la concateno
		s = strstr(p_param->file_out.val.s,".wav");
		if ( s == NULL )
		{
			strcat( p_param->file_out.val.s, ".wav" );
		}
	}
	else
	{
		//	Se non è stato passato come parametro uso il valore file in a cui sostituisco l'estensione .csv con .wav
		p_param->file_out.pos 	= -1;
		p_param->file_out.val.s = (char*)calloc( 1, strlen( p_param->file_in.val.s + 5 ) );
		strcpy( p_param->file_out.val.s, p_param->file_in.val.s );
		//	Se non trovo l'estensione .csv concateno .wav, altrimenti la sostituisco
		s = strstr(p_param->file_out.val.s,".csv");
		if ( s == NULL )
		{
			strcat( p_param->file_out.val.s, ".wav" );
		}
		else
		{
			strncpy(s,".wav",4);
		}
	}

	return 1;
}

int main(int argc, char **argv)
{
	int			i_ind;
	int			i_num_token;
	int         i_num_data_row;
	s_param		Parametri;
	s_csv_file	csv_file;
	s_dati		Dati;
	s_wav_file	wav_file;
	
	//	Validazione parametri input
	if ( !ParseParam( argc, argv, &Parametri ) )
	{
		Istruzioni( argc, argv );
		return 1;
	}
	
	InitCsv( &csv_file );

	//	------------------------
	
	csv_file.nome_file = (char*)calloc( 1, strlen( Parametri.file_in.val.s ) + 1 );
	strcpy( csv_file.nome_file, Parametri.file_in.val.s );

	if ( Parametri.flags.f_verbose ) {
		printf( "\nRigolCsvToWav - Versione %s\n\n", S_VERSION );
		fflush(stdout);
	}

	if ((csv_file.fd = fopen64( csv_file.nome_file, "r" )) == NULL )
	{
		printf( "\nErrore apertura file csv.\n\n" );
		return 1;
	}

	if ( Parametri.flags.f_verbose ) {
		printf( "   Leggo il file per calcolare i valori da normalizzare.\n\n");
		fflush(stdout);
	}
		
	//	Skip Header CSV
	if ( Parametri.flags.f_very_verbose )
	{
		printf( "Header:\n" );
	}
	i_num_data_row = 0;
	for ( i_ind = 0; i_ind < Parametri.skip_rows.val.i; i_ind++ )
	{
		//	Leggo una riga di header
		LeggiCsv( &csv_file );
		//	Visualizzo dati presenti nell' header csv
		if ( Parametri.flags.f_very_verbose )
		{
			i_num_token = ContaColonneCsv( &csv_file );		//	Nell'header il numero di colonne può cambiare
			InitColonneCsv( &csv_file, i_num_token );
			TokenizzaCsv( &csv_file );
			PrintCsv( &csv_file );
			printf( "\n" );
		}

		i_num_data_row++;
	}

	//	Leggo i valori
	i_num_data_row = 0;
	while ( !feof( csv_file.fd ) )
	{

		//	Leggo una riga di dati
		LeggiCsv( &csv_file );
		if ( strlen( csv_file.s ) > 0 )
		{
			//	Conto il numero di colonne e riservo la memoria per i dati
			if ( i_num_data_row == 0 )
			{
				i_num_token = ContaColonneCsv( &csv_file );
				InitColonneCsv( &csv_file, i_num_token );
				InitDati( &Dati, i_num_token );
			}
			//	Conto i token validi presenti nella riga appena letta
			i_num_token = TokenizzaCsv( &csv_file );
			if ( i_num_token == Dati.num_canali )
			{
				//	Converto le stringhe nelle colonne csv in numero nei dati,
				//		tenendo traccia di min e max e numero di dati letti
				//		nelle statistiche
				ConvertiDati( &csv_file, &Dati );
				//	Visualizzo i valori letti ed i valori convertiti
				if ( Parametri.flags.f_very_verbose )
				{
					if ( i_num_data_row == 0 )
					{
						printf( "Dati:\n" );
						for ( i_ind = 0; i_ind < Dati.num_canali; i_ind++ )
						{
							printf( "      Csv Col % 1d ", i_ind );
						}
						printf( "| " );
						for ( i_ind = 0; i_ind < Dati.num_canali; i_ind++ )
						{
							printf( "   Chan % 1d ", i_ind );
						}
						printf( "\n" );
					}
					PrintCsv( &csv_file );
					printf( "| " );
					PrintDati( &Dati );
					printf( "\n" );
				}
			}
			else
			{
				if ( Parametri.flags.f_verbose ) {
					printf( "   Attenzione: riga %d, numero di dati per riga errato.\n", (i_num_data_row + Parametri.skip_rows.val.i + 1) );
					fflush(stdout);
				}
			}
		}
		else
		{
			if ( Parametri.flags.f_verbose ) {
				printf( "   Attenzione: riga %d vuota.\n", (i_num_data_row + Parametri.skip_rows.val.i + 1) );
				fflush(stdout);
			}
		}
		
		i_num_data_row++;
	}

	//	Statistiche
	CalcolaCmAmp( &Dati );
	if ( Parametri.flags.f_verbose ) {
		PrintStatsDati( &Dati );
		fflush(stdout);
	}
	
	//	-------------------------------------------------------------
	
	//	Rileggo il file

	//	-------------------------------------------------------------
	
	rewind( csv_file.fd );
	fflush( csv_file.fd );
	
	//	------------------------
	
	wav_file.nome_file = (char*)calloc( 1, strlen( Parametri.file_out.val.s ) + 1 );
	strcpy( wav_file.nome_file, Parametri.file_out.val.s );

	if ( Parametri.flags.f_verbose ) {
		printf( "   File in : %s\n", csv_file.nome_file );
		printf( "   File out: %s\n\n", wav_file.nome_file );
		fflush(stdout);
	}

//	Strutture dati file wav
#ifndef USE_SNDFILE

	if ((wav_file.fd = fopen(wav_file.nome_file, "w")) == NULL )
	{
		printf( "\nErrore apertura file wav.\n\n" );
		return 1;
	}

	//	Scrittura Header RIFF file Wav
	wav_file.wh = (s_wav_riff_hd*) calloc( 1, sizeof(s_wav_riff_hd) );
	
	memcpy( wav_file.wh->riff_cd.ChunkID, "RIFF", 4 );
	wav_file.wh->riff_cd.ChunkSize		= 0;
	memcpy( wav_file.wh->riff_cd.Format, "WAVE", 4 );
	
	memcpy( wav_file.wh->fmt_sc.Subchunk1ID, "fmt ", 4 );
	wav_file.wh->fmt_sc.Subchunk1Size	= 16;
	wav_file.wh->fmt_sc.AudioFormat		= 1;
	wav_file.wh->fmt_sc.NumChannels		= Dati.num_canali;
	wav_file.wh->fmt_sc.SampleRate		= Parametri.sample_rate.val.i;
	wav_file.wh->fmt_sc.BitsPerSample	= 16;
	wav_file.wh->fmt_sc.BlockAlign		= wav_file.wh->fmt_sc.NumChannels * wav_file.wh->fmt_sc.BitsPerSample / 8;
	wav_file.wh->fmt_sc.ByteRate		= wav_file.wh->fmt_sc.SampleRate * wav_file.wh->fmt_sc.BlockAlign;

	memcpy( wav_file.wh->data_sc.Subchunk2ID, "data", 4 );
	wav_file.wh->data_sc.Subchunk2Size	= Dati.num_dati * wav_file.wh->fmt_sc.NumChannels * wav_file.wh->fmt_sc.BitsPerSample/8;

	wav_file.wh->riff_cd.ChunkSize		= 36 + wav_file.wh->data_sc.Subchunk2Size;

	wav_file.wh->Data					= (short*) calloc( 1, wav_file.wh->data_sc.Subchunk2Size );
	wav_file.wh->pData					= wav_file.wh->Data;
	
#else

	wav_file.sfinfo.format		= SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	wav_file.sfinfo.channels	= Dati.num_canali;
	wav_file.sfinfo.samplerate	= Parametri.sample_rate.val.i;

#endif

//	Apertura file wav in scrittura
#ifndef USE_SNDFILE

	if ((wav_file.fd = fopen(wav_file.nome_file, "w")) == NULL )
	{
		printf( "\nErrore apertura file wav.\n\n" );
		return 1;
	}

	fwrite( &wav_file.wh->riff_cd, sizeof( s_riff_cd ), 1, wav_file.fd );
	fflush( wav_file.fd );

	fwrite( &wav_file.wh->fmt_sc, sizeof( s_fmt_sc ), 1, wav_file.fd );
	fflush( wav_file.fd );

	fwrite( &wav_file.wh->data_sc, sizeof( s_data_sc ), 1, wav_file.fd );
	fflush( wav_file.fd );

#else
	
	if ((wav_file.fd = sf_open(wav_file.nome_file, SFM_WRITE, &wav_file.sfinfo)) == NULL )
	{
		printf( "\nErrore apertura file wav.\n\n" );
		return 1;
	}
	
#endif

	//	Skip Header CSV
	for ( i_ind = 0; i_ind < Parametri.skip_rows.val.i; i_ind++ )
	{
		LeggiCsv( &csv_file );
	}

	if ( Parametri.flags.f_verbose ) {
		printf( "   Scrivo file wav.\n\n");
		fflush(stdout);
	}
		
	i_num_data_row = 0;
	while ( !feof( csv_file.fd ) )
	{
		//	Leggo una riga di dati
		LeggiCsv( &csv_file );
		
		//	
		i_num_token = TokenizzaCsv( &csv_file );
		if ( i_num_token == Dati.num_canali )
		{
			ConvertiDati( &csv_file, &Dati );
			NormalizzaDati( &Dati );

#ifndef USE_SNDFILE
			memcpy( wav_file.wh->pData, Dati.f_dato_norm, Dati.num_canali*sizeof(short) );
			wav_file.wh->pData += Dati.num_canali;
#else
			sf_writef_short( wav_file.fd, Dati.f_dato_norm, 1);
#endif

		}

		i_num_data_row++;
	}

#ifndef USE_SNDFILE
	fwrite( wav_file.wh->Data, wav_file.wh->data_sc.Subchunk2Size, 1, wav_file.fd );
	fflush( wav_file.fd );
#endif
	
	//	Chiusura files
	if ( Parametri.flags.f_verbose ) {
		printf( "   Ho finito.\n\n");
		fflush(stdout);
	}
		
	if ( csv_file.fd != NULL )
		fclose( csv_file.fd );
	
#ifndef USE_SNDFILE
	if ( wav_file.fd != NULL )
		fclose( wav_file.fd );
#else
 	if ( wav_file.fd != NULL )
		sf_close( wav_file.fd );		
#endif
		
	//	Fine
	return 0;
}
