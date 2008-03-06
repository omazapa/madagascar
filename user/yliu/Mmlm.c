/* 2D Multistage median filtering. */
/*
  Copyright (C) 2007 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <rsf.h>
#include <stdio.h>
#include <math.h>

#include "median.h"

static void extenddata(float* tempt,float* extendt,int nfw,int n1,int n2,bool boundary);
/*extend seismic data*/

int main (int argc, char* argv[]) 
{
	int n1,n2,n3; /*n1 is trace length, n2 is the number of traces, n3 is the number of 3th axis*/
	int i,j,k,pass,ii;
        int nfw;    /*nfw is the filter-window length*/
	int m;
	float a;   /*temporary variable*/
        bool boundary;
        /**/
	float *trace;
	float *tempt;
	float *extendt;
	float *temp1,*temp2,*temp3,*temp4;
	float *z,*Y;
        sf_file in, out;

        sf_init (argc, argv); 
        in = sf_input("in");
        out = sf_output("out");
    
        if (!sf_histint(in,"n1",&n1)) sf_error("No n1= in input");
        n2 = sf_leftsize(in,1);
	/* get the trace length (n1) and the number of traces (n2)*/

        if (!sf_getint("nfw",&nfw)) sf_error("Need integer input");
        /* filter-window length (positive and odd integer)*/

        if (!sf_getbool("boundary",&boundary)) boundary=false;
        /* if y, boundary is data, whereas zero*/

        if (nfw < 1)  sf_error("Need positive integer input"); 
        if (nfw%2 == 0)  nfw = (nfw+1);
        m=(nfw-1)/2;

        trace = sf_floatalloc(n1*n2);
	tempt = sf_floatalloc(n1*n2);
	extendt = sf_floatalloc((n1+2*m)*(n2+2*m));
	temp1 = sf_floatalloc(nfw);
	temp2 = sf_floatalloc(nfw);
	temp3 = sf_floatalloc(nfw);
	temp4 = sf_floatalloc(nfw);
	z = sf_floatalloc(4);
	Y = sf_floatalloc(3);


        for(ii=0;ii<n3;ii++)
        {
		sf_floatread(trace,n1*n2,in);

		for(i=0;i<n1*n2;i++)
		{
			tempt[i]=trace[i];
		}

		extenddata(tempt,extendt,nfw,n1,n2,boundary);

	   /************2D multistage median filter****************/
		for(i=m;i<(n2+m);i++)
		{
			for(j=m;j<(n1+m);j++)
			{
				for(k=0;k<nfw;k++)
				{
					temp1[k]=extendt[(n1+2*m)*i+(j-m+k)];     /*vertical*/
					temp2[k]=extendt[(n1+2*m)*(i-m+k)+j];    /*horizontal*/
					temp3[k]=extendt[(n1+2*m)*(i-m+k)+(j-m+k)];  /*left-up to right-down*/
					temp4[k]=extendt[(n1+2*m)*(i-m+k)+(j+m-k)];   /*left-down to right-up*/
				}
				z[0]=medianfilter(temp1,nfw);
				z[1]=medianfilter(temp2,nfw);
				z[2]=medianfilter(temp3,nfw);
				z[3]=medianfilter(temp4,nfw);

				for(pass=1;pass<4;pass++)
				{
					for(int jj=0;jj<4-pass;jj++)
					{
						if(z[jj]>z[jj+1])
						{
							a=z[jj];
							z[jj]=z[jj+1];
							z[jj+1]=a;
						}
					}
				}
				Y[0]=z[3];
				Y[1]=z[0];
				Y[2]=extendt[(n1+2*m)*i+j];
				trace[n1*(i-m)+j-m]=medianfilter(Y,3);
			}
		}

		sf_floatwrite(trace,n1*n2,out);
	}

    exit (0);
}

static void extenddata(float* tempt,float* extendt,int nfw,int n1,int n2,bool boundary)
/*extend seismic data*/
{
    int m=(nfw-1)/2;
    int i,j;

	for(i=0;i<(n1+2*m)*(n2+2*m);i++)
	{
		extendt[i]=0.0;
	}
	/*extend trace*/
	for(i=0;i<m;i++)
	{
		for(j=0;j<n1;j++)
		{
                        if (boundary)
			{
                            extendt[(n1+2*m)*i+j+m]=tempt[n1*0+j];
			}
                        else
                        {
                            extendt[(n1+2*m)*i+j+m]=0.0;
                        }
		}
	}
	for(i=0;i<n2;i++)
	{
		for(j=0;j<n1;j++)
		{
			extendt[(n1+2*m)*(i+m)+j+m]=tempt[n1*i+j];
		}
	}
	for(i=0;i<m;i++)
	{
		for(j=0;j<n1;j++)
		{
                        if (boundary)
			{
			    extendt[(n1+2*m)*(i+m+n2)+j+m]=tempt[n1*(n2-1)+j];
			}
                        else
                        {
                            extendt[(n1+2*m)*(i+m+n2)+j+m]=0.0;
                        }
		}
	}
	/*extend the number of samples*/
	for(i=0;i<(n2+2*m);i++)
	{
		for(j=0;j<m;j++)
		{
                        if (boundary)
			{
			    extendt[(n1+2*m)*i+j]=extendt[(n1+2*m)*i+m];
			}
                        else
                        {
                            extendt[(n1+2*m)*i+j]=0.0;
                        }
		}
	}
	for(i=0;i<(n2+2*m);i++)
	{
		for(j=0;j<m;j++)
		{
                        if (boundary)
			{
			    extendt[(n1+2*m)*i+j+n1+m]=extendt[(n1+2*m)*i+n1+m-1];
			}
                        else
                        {
                            extendt[(n1+2*m)*i+j+n1+m]=0.0;
                        }
		}
	}
	
}


/* 	$Id: Mmlm.c 3351 2008-03-05 20:36:10Z yang $	 */


