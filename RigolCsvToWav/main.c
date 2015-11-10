#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>

#define FLT_MAX		__FLT_MAX__
#define F_DEBUG

typedef struct {
	FILE*	fd;
	int		num_colonne;
	char	colonna[6][32];
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
	float	f_dato[4];
	int		i_dato[4];
	s_stat	stat[4];
} s_dati;

typedef struct {
	char	file[256];
	char*	nome_file;
	int		sample_rate;
} s_param;

typedef struct {
	SNDFILE*	fd;
	SF_INFO 	sfinfo;
	char		nome_file[256];
} s_wav_file;

void InitCsv( s_csv_file* p_csv )
{
	int			i_ind;
	
	p_csv->num_colonne 	= 0;
	for ( i_ind=0; i_ind<6; i_ind++ )
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
	for ( i_ind=0; i_ind<4; i_ind++ )
	{
		p_dati->f_dato[ i_ind ]		= 0.0;
		p_dati->stat[ i_ind ].f_max =-FLT_MAX;
		p_dati->stat[ i_ind ].f_min = FLT_MAX;
		p_dati->stat[ i_ind ].f_cm	= 0.0;
		p_dati->stat[ i_ind ].f_amp	= 0.0;
		p_dati->i_dato[ i_ind ]		= 0;
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
	
	for ( i_ind=0; i_ind < 4; i_ind++ )
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
	
 	for ( i_ind=0; i_ind < 4; i_ind++ )
	{
		p_dati->stat[i_ind].f_cm = ( p_dati->stat[i_ind].f_max + p_dati->stat[i_ind].f_min ) / 2.0;
		p_dati->stat[i_ind].f_amp = ( p_dati->stat[i_ind].f_max - p_dati->stat[i_ind].f_min ) / 2.0;
	}
}

void NormalizzaInt( s_dati* p_dati )
{
	int	i_ind;
	
 	for ( i_ind=0; i_ind < 4; i_ind++ )
	{
		p_dati->i_dato[i_ind] = ( ( p_dati->f_dato[i_ind] - p_dati->stat[i_ind].f_cm ) / p_dati->stat[i_ind].f_amp ) * ( 0x7FFF0000 );
	}
}

int main(int argc, char **argv)
{
	s_param		Parametri;
	
	s_csv_file	Csv;
	s_dati		Dati;
	
	s_wav_file	wav_file;

	InitCsv( &Csv );
	InitDati( &Dati );

#ifndef F_DEBUG	
	if ( argc != 3 )
	{
		printf( "\nIstruzioni:\n" );
		printf( "	RigolCsvToWav.exe file_in sample_rate\n" );
		printf( "	Es: RigolCsvToWav file_in.csv 125000000\n" );
		return 1;
	}
#endif		
	
#ifdef F_DEBUG	
	strcpy( Parametri.file, "in.csv" );
	Parametri.sample_rate = 44100;
	
	strcpy( wav_file.nome_file, "out.wav" );
#else		
	strcpy( Parametri.file, argv[1] );
	sscanf( argv[2], "%d", &Parametri.sample_rate );

	Parametri.nome_file = strtok( Parametri.file, ".csv" );
	
	strcpy( wav_file.nome_file, Parametri.nome_file );
	strcat( wav_file.nome_file, ".wav" );

	strcpy( Parametri.file, argv[1] );
#endif		

	if ((Csv.fd = fopen64( Parametri.file, "r" )) == NULL )
	{
		printf( "Errore apertura file.\n" );
		return 1;
	}
	
	LeggiCsv( &Csv );
	PrintCsv( &Csv );
	LeggiCsv( &Csv );
	PrintCsv( &Csv );

	while ( !feof( Csv.fd ) )
	{
		LeggiCsv( &Csv );
		ConvertiDati( &Csv, &Dati );
	}
	
	CalcolaCmAmp( &Dati );
	
	//	-------------------------------------------------------------
	
	rewind( Csv.fd );
	
	wav_file.sfinfo.format		= SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	wav_file.sfinfo.channels	= Dati.num_canali;
	wav_file.sfinfo.samplerate	= Parametri.sample_rate;
	
	wav_file.fd = sf_open(wav_file.nome_file, SFM_WRITE, &wav_file.sfinfo);
	
	LeggiCsv( &Csv );
	LeggiCsv( &Csv );
	while ( !feof( Csv.fd ) )
	{
		LeggiCsv( &Csv );
		ConvertiDati( &Csv, &Dati );
		NormalizzaInt( &Dati );

		sf_writef_int( wav_file.fd, Dati.i_dato, 1);
	}
	
	if ( Csv.fd != NULL )
		fclose( Csv.fd );
	
	if ( wav_file.fd != NULL )
		sf_close( wav_file.fd );
		
	return 0;
}
