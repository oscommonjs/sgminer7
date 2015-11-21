/*******************************************************************************
* Jojo's Patch: apply patch from jdiff to a binary file
*
* Copyright (C) 2002-2009 Joris Heirbaut
*
* + some simplifications for WePayBTC project
*
* Author                Version Date       Modification
* --------------------- ------- -------    -----------------------
* Joris Heirbaut        v0.0    10-06-2002 hashed compare
* Joris Heirbaut        v0.1    20-06-2002 optimized esc-sequences & lengths
* Joris Heirbaut        v0.4c   09-01-2003 use seek for DEL-instruction
* Joris Heirbaut        v0.5    13-05-2003 do not count past EOF
* Joris Heirbaut        v0.6    13-05-2005 large-file support
* Joris Heirbaut        v0.7    01-09-2009 large-file support
* Joris Heirbaut        v0.7    29-10-2009 use buffered reading
* Joris Heirbaut        v0.7    01-11-2009 do not read original file on MOD
* Joris Heirbaut		v0.8	15-09-2011 C++ wrapping
*
* Licence
* -------
*
* This program is free software. Terms of the GNU General Public License apply.
*
* This program is distributed WITHOUT ANY WARRANTY, without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.a
*
* A copy of the GNU General Public License if found in the file "Licence.txt"
* deliverd along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
* Parts or all of this source code may only be reused within other GNU free, open
* source, software.
* So if your project is not an open source project, you are non entitled to read
* any further below this line!
*
*******************************************************************************/

/**
 * Output routine constants
 */
#define ESC     0xA7    /* Escape       */
#define MOD     0xA6    /* Modify       */
#define INS     0xA5    /* Insert       */
#define DEL     0xA4    /* Delete       */
#define EQL     0xA3    /* Equal        */
#define BKT     0xA2    /* Backtrace    */

/*******************************************************************************
* Input routines
*******************************************************************************/
#define ESC     0xA7
#define MOD     0xA6
#define INS     0xA5
#define DEL     0xA4
#define EQL     0xA3
#define BKT     0xA2

off_t ufGetInt( FILE *lpFil ){
  off_t liVal ;

  liVal = getc(lpFil) ;
  if (liVal < 252)
    return liVal + 1 ;

  if (liVal == 252)
    return 253 + getc(lpFil) ;

  if (liVal == 253) {
    liVal = getc(lpFil) ;
    liVal = (liVal << 8) + getc(lpFil) ;
    return liVal ;
  }

  if (liVal == 254) {
    liVal = getc(lpFil) ;
    liVal = (liVal << 8) + getc(lpFil) ;
    liVal = (liVal << 8) + getc(lpFil) ;
    liVal = (liVal << 8) + getc(lpFil) ;
    return liVal ;
  }

  //fprintf(stderr, "64-bit length numbers not supported!\n");
  return 0;
}

/*******************************************************************************
* Patch function
*******************************************************************************
* Input stream consists of a series of
*   <op> (<data> || <len>)
* where
*   <op>   = <ESC> (<MOD>||<INS>||<DEL>||<EQL>)
*   <data> = <chr>||<ESC><ESC>
*   <chr>  = any byte different from <ESC><MOD><INS><DEL> or <EQL>
*   <ESC><ESC> yields one <ESC> byte
*******************************************************************************/
int jpatch ( FILE *asFilOrg, FILE *asFilPch, unsigned char *pOutBuf, int iOutMaxLen )
{
  int liInp ;         /* Current input from patch file          */
  int liOpr ;         /* Current operand                        */
  off_t lzOff ;       /* Current operand's offset               */
  off_t lzMod = 0;    /* Number of bytes to skip on MOD         */
  int lbChg=false ;   /* Changing operand?                      */
  int lbEsc=false ;   /* Non-operand escape char found?	        */
  int iCurOutLen = 0;

  liOpr = ESC ;
  while ((liInp = getc(asFilPch)) != EOF) {
    if(iCurOutLen >= iOutMaxLen)
		return -1;//overflow

	// Parse an operator: ESC liOpr [lzOff]
    if (liInp == ESC) {
      liInp = getc(asFilPch);
      switch(liInp) {
        case MOD:
          liOpr = MOD;
          lbChg = true;
          break ;

        case INS:
          liOpr = INS;
          lbChg = true;
          break ;

        case DEL:
          liOpr = DEL;
          lzOff = ufGetInt(asFilPch);

          if (fseek(asFilOrg, lzOff + lzMod, SEEK_CUR) != 0) {
			  return -2;//fprintf(stderr, "Could not position on original file (seek %"PRIzd" + %"PRIzd").\n", lzOff, lzMod);
          }
          lzMod = 0;
          lbChg = true;
          break ;

        case EQL:
          liOpr = EQL;
          lzOff = ufGetInt(asFilPch);

          if (lzMod > 0) {
              if (fseek(asFilOrg, lzMod, SEEK_CUR) != 0) {
                  return -3;//fprintf(stderr, "Could not position on original file (skip %"PRIzd").\n", lzMod);
              }
              lzMod = 0;
          }

		  if((iCurOutLen + lzOff) > iOutMaxLen)
			  return 0;

          if (fread(pOutBuf + iCurOutLen, 1, lzOff, asFilOrg) != lzOff)
              return -4;//fprintf(stderr, "Error reading original file.\n");

          iCurOutLen += lzOff;
          lbChg = true;
          break ;

        case BKT:
          liOpr = BKT ;
          lzOff = ufGetInt(asFilPch) ;

          if (fseek(asFilOrg, lzMod - lzOff, SEEK_CUR) != 0) {
            return -5;//fprintf(stderr, "Could not position on original file (seek back %"PRIzd" - %"PRIzd").\n", lzMod, lzOff);
          }
          lzMod = 0 ;
          lbChg = true;
          break ;

        case ESC:
          break;

        default:
          lbEsc = true;
          break;
      }
    }

    if ( lbChg ) {
      lbChg = false ;
    } else {
      switch (liOpr) {
        case DEL: break;
        case EQL: break;
        case BKT: break;
        case MOD:
          if (lbEsc) {
            pOutBuf[iCurOutLen++] = ESC;
            lzMod ++ ;
          }

		  pOutBuf[iCurOutLen++] = liInp;
          lzMod ++ ;
          break ;

        case INS :
          if (lbEsc) {
            pOutBuf[iCurOutLen++] = ESC;
          }

		  pOutBuf[iCurOutLen++] = liInp;
          break ;
      }
    } /* if lbChg */

    lbEsc = false ;
  } /* while */

  return iCurOutLen;
}
