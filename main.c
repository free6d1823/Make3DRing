#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

void usage(char* name)
{
    printf("Usage: %s [ <output_h>] [-v <var_name>] [-t <type>]\n", name);
    printf("\t<output_h>    output header filename.\n");
    printf("\t-r <section>     redius of section.\n");
    printf("\t-R <ring>        redius of ring.\n");
	printf("\t-m <samples>     samples of outter ring. Default 64.\n");
	printf("\t-n <sec_samp>    samples per section. Default 24.\n");
	printf("\t-v <var_name>    variable name\n");
	printf("\t-t <type>        ring type, 0: flat, 1: vertival\n");
}
int gnEdges = 64; /* edges of outer ring */
int gnSamples = 24; /* samples per section*/
double rSection = 1.00;
double rRing = 10.0;
int gType=0;


void printHeader(FILE* fp, char* name)
{
	time_t T= time(NULL);
	struct  tm tm = *localtime(&T);

	fprintf(fp, "/*********************************************************************\n");
	fprintf(fp, "\tCopyright (C) 2020  Siengine Inc.,\n");

	fprintf(fp, "\tDate: %04d/%02d/%02d %02d:%02d:%02d  \n", 
			tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(fp, " **********************************************************************/\n\n");

	fprintf(fp, "#ifndef _%s_H\n", name);
	fprintf(fp, "#define _%s_H\n", name);
}
typedef struct _vec3{
	double x; double y; double z;
}vec3;
typedef struct _vec2{
    double u; double v;
}vec2;

#define PI M_PI
int printSection0(vec3* data, double grad)
{
	int j;
	double deAngle = 2*PI/(double)gnSamples;
	double angle=0;
	for (j=0; j< gnSamples; j++) {
		data[j].x= rSection* sin(angle) + rRing;
		data[j].y = rSection* cos(angle);
		data[j].z = 0;
		/*rotate y-axis grad */
		data[j].z = - data[j].x*sin(grad);
		data[j].x = data[j].x*cos(grad) ;
		angle += deAngle;
	}
	return j;
}
int printSection1(vec3* data, double grad)
{
	int i;
	double deAngle = 2*PI/(double)gnSamples;
	double angle=0;
	for (i=0; i< gnSamples; i++) {
		data[i].y= rSection* sin(angle) + rRing;
		data[i].z = rSection* cos(angle);
		data[i].x = 0;
		/*rotate z-axis grad */
		data[i].x = - data[i].y*sin(grad);
		data[i].y = data[i].y*cos(grad);
		angle += deAngle;
	}
	return i;
}
void MapTexture(vec3* pV, vec2* pT, int total)
{
	int i;
	vec2 min = {0};
	vec2 max = {0};
	for (i=0; i< total; i++) {
		pT[i].u = pV[i].x;
		pT[i].v = pV[i].y;
		if (pT[i].u > max.u) max.u=pT[i].u;
        if (pT[i].v > max.v) max.v=pT[i].v;
        if (pT[i].u < min.u) min.u=pT[i].u;
        if (pT[i].v < min.v) min.v=pT[i].v;
	}
	vec2 offset = {-min.u, -min.v};
	vec2 scale = {max.u-min.v, max.v-min.v};
    for (i=0; i< total; i++) {
        pT[i].u = (offset.u + pT[i].u)/scale.u;
        pT[i].v = (offset.v + pT[i].v)/scale.v;
	}
}
/* return length of elements */
int printData(FILE* fpOut)
{
	int i,j;
	double deGradient = 2* PI /(double)gnEdges;
	double gradient = 0;
    int nTotalSamples = gnSamples * gnEdges;
    vec3* vertex = (vec3*) malloc(nTotalSamples* sizeof(vec3));
	vec2* texture = (vec2*) malloc(nTotalSamples* sizeof(vec2));
	vec3* pVer = vertex;
	for (i=0; i<gnEdges; i++) {
        if (gType == 0)
    		printSection0(pVer, gradient);
        else
    		printSection1(pVer, gradient);

		gradient += deGradient;
		pVer += gnSamples;
	}
	/* print vertex */
    fprintf(fpOut, "static float3D %s[] = {\n", "gpVertexBuf");
	pVer = vertex;
    for (i=0; i<gnEdges; i++) {
        fprintf(fpOut, "         /* section %d */\n", i);
		for (j=0; j< gnSamples; j++) {
			fprintf(fpOut, "    {%12.8f, %12.8f, %12.8f}, \n", pVer[j].x, pVer[j].y, pVer[j].z);
		}
		pVer += gnSamples;
	}
    fprintf(fpOut, "};\n\n");

	MapTexture(vertex, texture, nTotalSamples);
	/* print texture */
    fprintf(fpOut, "static float2D %s[] = {\n", "gpUvTexture");
    vec2* pT = texture;
	for (i=0; i<gnEdges; i++) {
        fprintf(fpOut, "         /* section %d */\n", i);
        for (j=0; j< gnSamples; j++) {
            fprintf(fpOut, "    {%12.8f, %12.8f}, \n", pT[j].u, pT[j].v);
        }
        pT += gnSamples;
    }    
	fprintf(fpOut, "};\n\n");
	/* index, one face 6 vertex */
	int nFaces = gnSamples * gnEdges;
	int p,q,r,s;	
    fprintf(fpOut, "static unsigned short %s[] = {\n", "gpIndices");
	p=0;
	for (i = 0; i<gnEdges; i++) {
		for(j=0; j<gnSamples; j++) {
			q=(p+1); if(q%gnSamples == 0) q=i * gnSamples;
			r=(p+gnSamples)%nFaces;
			s=(q+gnSamples)%nFaces;
			fprintf(fpOut, "    %5d,%5d,%5d,%5d,%5d,%5d, /* face(%d,%d) */\n", p,p,r,q,s,s,i,j);
			p++;
		}
	}
    fprintf(fpOut, "};\nstatic int mNumToDraw = %d;\n", nFaces*6);
	fprintf(fpOut, "\n");
	free(vertex);
	free(texture);
	return nTotalSamples;
}
void printEnd(FILE* fp, char* name)
{
	fprintf(fp, "#endif //_%s_\n", name);
}
int main(int argc, char *argv[])
{
	FILE* fpOut = NULL;

	char* szOutFile = NULL;
	char szVar[64] = "data";
	char szName[64] = "VERTEX_DATA";
	char ch;
	while ((ch = getopt(argc, argv, "m:n:f:v:r:R:t:h?")) != -1)
	{
		switch (ch)
		{
		case 'f':
		    szOutFile = optarg;
		    break;
		case 'v':
		    strncpy((char*)szVar, optarg, 63);
		    break;
		case 'R':
			rRing = (double) atoi(optarg)/10.0;
			break;
		case 'r':
			rSection = (double) atoi(optarg)/10.0;
            break;
        case 't':
            gType = atoi(optarg);
            break;
		case 'm':
			gnEdges = atoi(optarg);
			break;
		case 'n':
			gnSamples = atoi(optarg);
			break;
		default:
		    usage(argv[0]);
		    exit(-1);
		}            
	}

    szOutFile = argv[optind];

    fpOut = fopen(szOutFile, "wt");
    if (fpOut == NULL)
        fpOut = stdout;

	/* print header */
	printHeader(fpOut, szName);
	int len=printData(fpOut);
	fprintf(fpOut, "/* Total %d elements */\n", len);
	printEnd(fpOut, szName);
	if(fpOut)
		fclose(fpOut);
    return 0;
}

