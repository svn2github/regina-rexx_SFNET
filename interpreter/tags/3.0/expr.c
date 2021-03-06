#ifndef lint
static char *RCSid = "$Id: expr.c,v 1.10 2002/03/23 01:30:34 mark Exp $";
#endif

/*
 *  The Regina Rexx Interpreter
 *  Copyright (C) 1993-1994  Anders Christensen <anders@pvv.unit.no>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "rexx.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

#define TRACEVALUE(a,b) if (TSD->trace_stat=='I') tracevalue(TSD,a,b)

#ifdef TRACEMEM
static void mark_in_expr( const tsd_t *TSD )
{
   if (TSD->rdes.num)
      markmemory( TSD->rdes.num, TRC_STATIC ) ;
   if (TSD->ldes.num)
      markmemory( TSD->ldes.num, TRC_STATIC ) ;
}
#endif


int init_expr( tsd_t *TSD )
{
#ifdef TRACEMEM
   regmarker( TSD, mark_in_expr ) ;
#endif
   TSD = TSD; /* keep compiler happy */
   return(1);
}


static num_descr *copy_num( const tsd_t *TSD, const num_descr *input )
{
   num_descr *new=NULL ;

   new = MallocTSD( sizeof( num_descr )) ;
   new->negative = input->negative ;
   new->size = input->size ;
   new->max = (input->max < 1) ? 1 : input->max ;
   new->exp = input->exp ;
   new->num = MallocTSD( new->max ) ;
   memcpy( new->num, input->num, new->size ) ;
   TSD = TSD; /* keep compiler happy */
   return new ;
}


static streng *num_to_str( const tsd_t *TSD, num_descr *input )
{
   return str_norm( TSD, input, NULL ) ;
}

static int num_to_bool( const num_descr *input )
{
   char ch=' ' ;

   if (input==NULL)
       exiterror( ERR_UNLOGICAL_VALUE, 0 )  ;

   if (input->size!=1 || input->negative || input->exp!=1)
       exiterror( ERR_UNLOGICAL_VALUE, 0 )  ;

    ch = input->num[0] ;
    if (ch!='0' && ch!='1')
        exiterror( ERR_UNLOGICAL_VALUE, 0 )  ;

    return ch=='1' ;
}

static int str_to_bool( const streng *input )
{
   char ch=' ' ;

   if (input->len!=1)
       exiterror( ERR_UNLOGICAL_VALUE, 0 )  ;

   ch = input->value[0] ;
   if (ch!='0' && ch!='1')
       exiterror( ERR_UNLOGICAL_VALUE, 0 )  ;

   return ch == '1' ;
}

static streng *bool_to_str( const tsd_t *TSD, int input )
{
   return Str_creTSD( input ? "1" : "0" ) ;
}


static num_descr *str_to_num( const tsd_t *TSD, const streng *input )
{
   return get_a_descr( TSD, input ) ;
}

static num_descr *bool_to_num( const tsd_t *TSD, int input )
{
   num_descr *num=NULL ;

   num = MallocTSD( sizeof( num_descr )) ;
   num->max = 8 ;
   num->num = MallocTSD( 8 ) ;
   num->size = 1 ;
   num->negative = 0 ;
   num->exp = 1 ;
   num->num[0] = (char) ((input) ? '1' : '0') ;
   return num ;
}

num_descr *calcul( tsd_t *TSD, nodeptr this, num_descr **kill )
{
   num_descr *numthr=NULL, *numone=NULL, *numtwo=NULL ;
   num_descr *ntmp1=NULL, *ntmp2=NULL ;
   int power=0 ;
   num_descr *nptr=NULL ;
   streng *sptr=NULL ;

   switch (/*(unsigned char)*/(this->type))
   {
      case 0:
      case 255:
      case X_MINUS:
         numone = calcul( TSD, this->p[0], &ntmp1 ) ;
         numtwo = calcul( TSD, this->p[1], &ntmp2 ) ;
         if (!ntmp2)
            ntmp2 = numtwo = copy_num( TSD, numtwo ) ;

         numtwo->negative = !numtwo->negative ;
         goto do_an_add ;

      case X_PLUSS:
         numone = calcul( TSD, this->p[0], &ntmp1 ) ;
         numtwo = calcul( TSD, this->p[1], &ntmp2 ) ;
do_an_add:
         if (ntmp1)
         {
            numthr = numone ;
            ntmp1 = NULL ;
         }
         else if (ntmp2)
         {
            numthr = numtwo ;
            ntmp2 = NULL ;
         }
         else
            numthr = copy_num( TSD, numtwo ) ;

         string_add( TSD, numone, numtwo, numthr ) ;
         break ;

      case X_MULT:
         numone = calcul( TSD, this->p[0], &ntmp1 ) ;
         numtwo = calcul( TSD, this->p[1], &ntmp2 ) ;
         if (ntmp1)
         {
            numthr = numone ;
            ntmp1 = NULL ;
         }
         else if (ntmp2)
         {
            numthr = numtwo ;
            ntmp2 = NULL ;
         }
         else
            numthr = copy_num( TSD, numtwo ) ;

         string_mul( TSD, numone, numtwo, numthr ) ;
         break ;

      case X_DEVIDE:
      case X_MODULUS:
      case X_INTDIV:
         numone = calcul( TSD, this->p[0], &ntmp1 ) ;
         numtwo = calcul( TSD, this->p[1], &ntmp2 ) ;
         if (numtwo->size==1 && numtwo->num[0]=='0')
             exiterror( ERR_BAD_ARITHMETIC, 0 )  ;

         numthr = copy_num( TSD, numtwo ) ;
         string_div( TSD, numone, numtwo, numthr, NULL,
            ((this->type==X_DEVIDE) ? DIVTYPE_NORMAL :
            ((this->type==X_MODULUS) ? DIVTYPE_REMINDER : DIVTYPE_INTEGER))) ;
         break ;

      case X_EXP:
         numone = calcul( TSD, this->p[0], &ntmp1 ) ;
         power = descr_to_int( calcul( TSD, this->p[1], &ntmp2 )) ;
         if (ntmp2)
         {
            numtwo = ntmp2 ;
/*            ntmp2 = NULL ; */
         }
         else
            ntmp2 = numtwo = copy_num( TSD, numone ) ;


         numthr = copy_num( TSD, numone ) ;
         string_pow( TSD, numone, power, numtwo, numthr ) ;
         break ;

      case X_STRING:
      case X_CON_SYMBOL:
         if (!this->u.number)
            this->u.number = get_a_descr( TSD, this->name ) ;

         if (TSD->trace_stat=='I')
            tracenumber( TSD, this->u.number, 'L' ) ;

         if (kill)
         {
            *kill = NULL ;
            return this->u.number ;
         }
         else
            return copy_num( TSD, this->u.number ) ;

      case X_SIM_SYMBOL:
      case X_STEM_SYMBOL:
         if (kill)
            *kill = NULL ;

         nptr = shortcutnum( TSD, this ) ;
         if (!nptr)
             exiterror( ERR_BAD_ARITHMETIC, 0 )  ;

         if (kill)
            return nptr ;
         else
            return copy_num( TSD, nptr ) ;

      case X_HEAD_SYMBOL:
         if (kill)
            *kill = NULL ;

         nptr = fix_compoundnum( TSD, this, NULL ) ;
         if (!nptr)
             exiterror( ERR_BAD_ARITHMETIC, 0 )  ;

         if (kill)
            return nptr ;
         else
            return copy_num( TSD, nptr ) ;

      case X_U_PLUSS:
      case X_U_MINUS:
         numthr = calcul( TSD, this->p[0], &ntmp1 ) ;
         if (!ntmp1)
            numthr = copy_num( TSD, numthr ) ;

         if (this->type==X_U_MINUS)
            numthr->negative = !numthr->negative ;

         if (kill)
            *kill = numthr ;

         if (TSD->trace_stat=='I')
            tracenumber( TSD, numthr, 'P' ) ;

         return numthr ;

      case X_IN_FUNC:
      case X_IS_INTERNAL:
      case X_IS_BUILTIN:
      case X_EX_FUNC:
      case X_IS_EXTERNAL:
      case X_CONCAT:
      case X_SPACE:
      {
         streng *stmp ;
         numthr = str_to_num( TSD, stmp=evaluate( TSD, this, &sptr )) ;
         if (sptr)
            Free_stringTSD( sptr ) ;
         if (kill)
            *kill = numthr ;
         return numthr ;
      }
      case X_LOG_NOT:
      case X_LOG_OR:
      case X_LOG_AND:
      case X_LOG_XOR:
      case X_S_DIFF:
      case X_S_EQUAL:
      case X_EQUAL:
      case X_GT:
      case X_LT:
      case X_GTE:
      case X_LTE:
      case X_DIFF:
      case X_SEQUAL:
      case X_SGT:
      case X_SLT:
      case X_SGTE:
      case X_SLTE:
      case X_SDIFF:
      case X_NEQUAL:
      case X_NGT:
      case X_NLT:
      case X_NGTE:
      case X_NLTE:
      case X_NDIFF:
      case X_S_NGT:
      case X_S_NLT:
      case X_S_GT:
      case X_S_GTE:
      case X_S_LT:
      case X_S_LTE:
         numthr = bool_to_num( TSD, isboolean( TSD, this )) ;
         if (kill)
            *kill = numthr ;
         return numthr ;

      default:
         exiterror( ERR_INTERPRETER_FAILURE, 1, __FILE__, __LINE__, "" )  ;
         return NULL ;
   }

   if (ntmp1)
   {
      FreeTSD( numone->num ) ;
      FreeTSD( numone ) ;
   }
   if (ntmp2)
   {
      FreeTSD( numtwo->num ) ;
      FreeTSD( numtwo ) ;
   }
   if (kill)
      *kill = numthr ;

   if (TSD->trace_stat=='I')
      tracenumber( TSD, numthr, 'O' ) ;

   str_strip( numthr ) ;
   str_round( numthr, TSD->currlevel->currnumsize ) ;
   return numthr ;
}



streng *evaluate( tsd_t *TSD, nodeptr this, streng **kill )
{
   streng *strone=NULL, *strtwo=NULL, *strthr=NULL ;
   streng *stmp1, *stmp2=NULL, *sptr=NULL ;
   streng *stmp=NULL ;
   const streng *cstmp=NULL ;
   num_descr *ntmp=NULL ;

   switch (/*(unsigned char)*/(this->type))
   {
      case 0:
      case 255:
      case X_PLUSS:
      case X_MINUS:
      case X_MULT:
      case X_DEVIDE:
      case X_MODULUS:
      case X_INTDIV:
      case X_EXP:
      case X_U_MINUS:
      case X_U_PLUSS:
         stmp = num_to_str( TSD, calcul( TSD, this, &ntmp )) ;
         if (ntmp)
         {
            FreeTSD( ntmp->num ) ;
            FreeTSD( ntmp ) ;
         }
         if (kill)
            *kill = stmp ;

         return stmp ;

      case X_NULL:
         return NULL ;

      case X_STRING:
      case X_CON_SYMBOL:
         cstmp = this->name ;
         if (TSD->trace_stat=='I')
            tracevalue( TSD, cstmp, 'L' ) ;

         goto cnsnt ;

      case X_HEAD_SYMBOL:
         /* always duplicate, since stmp might point to tmp area */
         stmp = Str_dupTSD( fix_compound(TSD, this, NULL)) ;
         if (kill)
            *kill = stmp ;
         return stmp ;

      case X_STEM_SYMBOL:
      case X_SIM_SYMBOL:
         cstmp = shortcut(TSD,this) ;
cnsnt:   if (kill)
         {
            *kill = NULL ;
            /* FIXME, FGC: I hate the following. The complete code
             * should be fixed here!
             */
            stmp = (streng *) cstmp;
         }
         else
            stmp = Str_dupTSD( cstmp ) ;
         return stmp ;

      case X_IN_FUNC:
      {
         nodeptr entry ;
         if ((entry=getlabel(TSD,this->name))!=NULL)
         {
            this->type = X_IS_INTERNAL ;
            this->u.node = entry ;
         }
         else
            this->u.node = NULL ;
      }

      case X_IS_INTERNAL:
      {
         proclevel oldlevel ;
         int stackmark ;
         nodeptr entry ;
         paramboxptr args ;
         streng *ptr ;

         /*
          * Check if the internal function name ends with a '.'.
          * This is an error in the ANSI standard, but it is possible
          * that existing code allows this, so only generate an error
          * if STRICT_ANSI OPTION is set.
          */
         if ( get_options_flag( TSD->currlevel, EXT_STRICT_ANSI ) )
         {
            if ( this->name->value[(this->name->len)-1] == '.' )
               exiterror( ERR_UNQUOTED_FUNC_STOP, 1, tmpstr_of( TSD, this->name ) )  ;
         }
         if ((entry=this->u.node)!=NULL)
         {
            nodeptr savecurrentnode;  /* pgb */
            set_sigl( TSD, TSD->currentnode->lineno ) ;
            oldlevel = TSD->currlevel ;
            args = initplist( TSD, this ) ;
            TSD->currlevel = newlevel( TSD, TSD->currlevel ) ;
            TSD->currlevel->args = args ;
            stackmark = pushcallstack(TSD,TSD->currentnode) ;

            savecurrentnode = TSD->currentnode; /* pgb */
            ptr = interpret( TSD, entry->next ) ;
            TSD->currentnode = savecurrentnode; /* pgb */

            if (ptr==NULL)
               exiterror( ERR_NO_DATA_RETURNED, 1, tmpstr_of( TSD, this->name ) )  ;

            popcallstack(TSD,stackmark) ;
            removelevel( TSD, TSD->currlevel ) ;
            TSD->currlevel = oldlevel ;
            TSD->currlevel->next = NULL ;
            TSD->trace_stat = TSD->currlevel->tracestat ;
            if (kill)
               *kill = ptr ;
            if (TSD->trace_stat=='I')
               tracevalue( TSD, ptr, 'F' ) ;

            return ptr ;
         }
      }
      /* THIS IS MEANT TO FALL THROUGH! */
      case X_IS_BUILTIN:
      case X_EX_FUNC:
      {
         streng *ptr ;

         if ((ptr=buildtinfunc( TSD, this )) != NOFUNC)
         {
            if (this->type != X_IS_BUILTIN)
               this->type = X_IS_BUILTIN ;
            if (kill)
               *kill = ptr ;

            if (!ptr)
                exiterror( ERR_NO_DATA_RETURNED, 1, tmpstr_of( TSD, this->name ) )  ;

            if (TSD->trace_stat=='I')
               tracevalue( TSD, ptr, 'F' ) ;

            return ptr ;
         }
         else
            this->type = X_IS_EXTERNAL ;
      }

      case X_IS_EXTERNAL:
      {
         streng *ptr, *command ;
         int stackmark ;
         paramboxptr args, targs ;

         if ( TSD->restricted )
            exiterror( ERR_RESTRICTED, 5 )  ;

         update_envirs( TSD, TSD->currlevel ) ;

         args = targs = initplist( TSD, this ) ;
#if 0
         command = Str_makeTSD( 1000 ) ;
         command = Str_catTSD(command,this->name ) ;
         for (;targs;targs=targs->next)
            if (targs->value)
            {
               command = Str_catstrTSD(command," ") ;
               command = Str_catTSD(command,targs->value) ;
            }

         stackmark = pushcallstack( TSD, TSD->currentnode ) ;
         ptr = execute_external(TSD,command,args,TSD->systeminfo->environment,
                                NULL,0, INVO_FUNCTION);
         popcallstack( TSD, stackmark ) ;
         deallocplink( TSD, args ) ;

         if (!ptr)
            ptr = run_popen( TSD, command, TSD->currlevel->environment ) ;

         if (!ptr)
         {
            exiterror( ERR_ROUTINE_NOT_FOUND, 0 )  ;
            ptr = nullstringptr() ;
         }
         if (kill)
            *kill = ptr ;

         if (TSD->trace_stat=='I')
            tracevalue( TSD, ptr, 'F' ) ;

         Free_stringTSD( command ) ;
#else

         stackmark = pushcallstack( TSD, TSD->currentnode ) ;
         ptr = execute_external(TSD,this->name,
                                args,
                                TSD->systeminfo->environment,
                                NULL,
                                TSD->systeminfo->hooks,
                                INVO_FUNCTION);
         popcallstack( TSD, stackmark ) ;

         if (!ptr)
         {
            /*
             * "this->name" wasn't a Rexx program, so
             * see if it is an OS command.
             * Only do this if the OPTIONS EXT_COMMANDS_AS_FUNCS is
             * set and STRICT_ANSI is NOT set.
             */
            if ( get_options_flag( TSD->currlevel, EXT_EXT_COMMANDS_AS_FUNCS ) 
            &&  !get_options_flag( TSD->currlevel, EXT_STRICT_ANSI ) )
            {
               command = Str_makeTSD( 1000 ) ;
               command = Str_catTSD(command,this->name ) ;
               for (;targs;targs=targs->next)
               {
                  if (targs->value)
                  {
                     command = Str_catstrTSD(command," ") ;
                     command = Str_catTSD(command,targs->value) ;
                  }
               }
               ptr = run_popen( TSD, command, TSD->currlevel->environment ) ;
               Free_stringTSD( command ) ;
            }
         }

         deallocplink( TSD, args ) ;

         if (!ptr)
         {
            exiterror( ERR_ROUTINE_NOT_FOUND, 1, tmpstr_of( TSD, this->name ) )  ;
            ptr = nullstringptr() ;
         }
         if (kill)
            *kill = ptr ;

         if (TSD->trace_stat=='I')
            tracevalue( TSD, ptr, 'F' ) ;

#endif
         return ptr ;
      }

      case X_CONCAT:
      case X_SPACE:
      {
         char *cptr ;

         strone = evaluate( TSD, this->p[0], &stmp1 ) ;
         strtwo = evaluate( TSD, this->p[1], &stmp2 ) ;
         strthr = Str_makeTSD(Str_len(strone)+Str_len(strtwo)+1) ;
         cptr = strthr->value ;
         memcpy( cptr, strone->value, strone->len ) ;
         cptr += strone->len ;
         if (this->type==X_SPACE)
            *(cptr++) = ' ' ;

         memcpy( cptr, strtwo->value, strtwo->len ) ;
         strthr->len = (cptr-strthr->value) + strtwo->len ;

         if (kill)
            *kill = strthr ;
         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;

         if (TSD->trace_stat=='I')
            tracevalue( TSD, strthr, 'O' ) ;


         return strthr ;
      }


      case X_LOG_NOT:
      case X_LOG_OR:
      case X_LOG_AND:
      case X_LOG_XOR:
      case X_S_DIFF:
      case X_S_EQUAL:
      case X_EQUAL:
      case X_GT:
      case X_LT:
      case X_GTE:
      case X_LTE:
      case X_DIFF:
      case X_SEQUAL:
      case X_SGT:
      case X_SLT:
      case X_SGTE:
      case X_SLTE:
      case X_SDIFF:
      case X_NEQUAL:
      case X_NGT:
      case X_NLT:
      case X_NGTE:
      case X_NLTE:
      case X_NDIFF:
      case X_S_NGT:
      case X_S_NLT:
      case X_S_GT:
      case X_S_GTE:
      case X_S_LT:
      case X_S_LTE:
         sptr = bool_to_str( TSD, isboolean( TSD, this )) ;
         if (kill)
            *kill = sptr ;
         return sptr ;

      default:
         exiterror( ERR_INTERPRETER_FAILURE, 1, __FILE__, __LINE__, "" )  ;
         return NULL ;
   }
}



int isboolean( tsd_t *TSD, nodeptr this )
{
   streng *strone=NULL, *strtwo=NULL ;
   streng *stmp1=NULL, *stmp2=NULL ;
   int tmp=0, sint=0 ;
   streng *sptr=NULL ;
   num_descr *ntmp=NULL ;

   switch (/*(unsigned char)*/(this->type))
   {
      case 0:
      case 255:
      case X_PLUSS:
      case X_MINUS:
      case X_MULT:
      case X_DEVIDE:
      case X_MODULUS:
      case X_INTDIV:
      case X_EXP:
      case X_U_MINUS:
      case X_U_PLUSS:
         tmp = num_to_bool( calcul( TSD, this, &ntmp )) ;
         if (ntmp)
         {
            FreeTSD( ntmp->num ) ;
            FreeTSD( ntmp ) ;
         }
         return tmp ;

      case X_STRING:
      case X_CON_SYMBOL:
         if (!this->u.number)
            this->u.number = get_a_descr( TSD, this->name ) ;
         return num_to_bool( this->u.number ) ;

      case X_SIM_SYMBOL:
      case X_STEM_SYMBOL:
         return num_to_bool( shortcutnum( TSD, this )) ;

      case X_HEAD_SYMBOL:
         return num_to_bool( fix_compoundnum( TSD, this, NULL )) ;

      case X_IN_FUNC:
      case X_IS_INTERNAL:
      case X_IS_BUILTIN:
      case X_EX_FUNC:
      case X_IS_EXTERNAL:
      case X_CONCAT:
      case X_SPACE:
         tmp = str_to_bool( sptr = evaluate( TSD, this, &sptr )) ;
         if (sptr)
            Free_stringTSD( sptr ) ;
         return tmp ;

      case X_LOG_NOT:
         sint = !isboolean( TSD, this->p[0] ) ;
         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'U' ) ;
         return sint ;

      case X_LOG_OR:
         sint = ( isboolean(TSD, this->p[0]) | isboolean( TSD, this->p[1] )) ;
         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'U' ) ;
         return sint ;

      case X_LOG_AND:
         sint = ( isboolean(TSD, this->p[0]) & isboolean( TSD, this->p[1] )) ;
         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'U' ) ;
         return sint ;

      case X_LOG_XOR:
         /* Well, sort of ... */
         sint = ( isboolean( TSD, this->p[0]) ^ isboolean( TSD, this->p[1] )) ;
         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'U' ) ;
         return sint ;

      case X_EQUAL:
      case X_DIFF:
      case X_GT:
      case X_GTE:
      case X_LT:
      case X_LTE:
      {
         int type ;
         compflags flags ;
         num_descr *rnum, *lnum ;
         streng *lval, *rval ;

         flags = this->u.flags ;
         rnum = lnum = 0 ;
         rval = lval = NULL ;
         stmp1 = stmp2 = NULL ;

         if (flags.lnum)
         {
            lnum = this->p[0]->u.number ;
            if (TSD->trace_stat=='I')
               tracenumber( TSD, lnum, 'L' ) ;
         }
         else if (flags.lsvar)
            lnum = shortcutnum( TSD, this->p[0] ) ;
         else if (flags.lcvar)
            lnum = fix_compoundnum( TSD, this->p[0], NULL ) ;

         if (!lnum)
            lval = evaluate( TSD, this->p[0], &stmp1 ) ;

         if (flags.rnum)
         {
            rnum = this->p[1]->u.number ;
            if (TSD->trace_stat=='I')
               tracenumber( TSD, rnum, 'L' ) ;
         }
         else if (flags.rsvar)
            rnum = shortcutnum( TSD, this->p[1] ) ;
         else if (flags.rcvar)
            rnum = fix_compoundnum( TSD, this->p[1], NULL ) ;

         if (!rnum)
            rval = evaluate( TSD, this->p[1], &stmp2 ) ;

         if (!lnum && !getdescr( TSD, lval, &TSD->ldes ))
            lnum = &TSD->ldes ;

         if (!rnum && !getdescr( TSD, rval, &TSD->rdes ))
            rnum = &TSD->rdes ;

         if (rnum && lnum)
            tmp = string_test( TSD, lnum, rnum ) ;
         else
         {
            char *s1, *s2, *e1, *e2 ;

            if (!lval)
            {
               assert( !stmp1 ) ;
               stmp1 = lval = str_norm( TSD, lnum, NULL ) ;
            }

            if (!rval)
            {
               assert( !stmp2 ) ;
               stmp2 = rval = str_norm( TSD, rnum, NULL ) ;
            }

            s1 = lval->value ;
            s2 = rval->value ;
            e1 = s1 + lval->len ;
            e2 = s2 + rval->len ;
            /*
             * Strip leading spaces - ignored in comparison
             */
            for(;(s1<e1)&&(isspace(*s1));s1++)
            {
               ;
            }
            for(;(s2<e2)&&(isspace(*s2));s2++)
            {
               ;
            }
            for (;(s1<e1)&&(s2<e2)&&(*s1==*s2);s1++,s2++)
            {
               ;
            }
            /*
             * Strip trailing spaces - ignored in comparison
             */
            /*  wait a bit! it is not (isspace(*s1)) we should test , but the end of string!  bja
            for (;(e1>s1)&&(isspace(*s1));e1--) ;
            for (;(e2>s2)&&(isspace(*s2));e2--) ;
            */
            for (;(e1>s1)&&(isspace(*(e1-1)));e1--) ;                                      /* bja */
            for (;(e2>s2)&&(isspace(*(e2-1)));e2--) ;                                      /* bja */

            if (s1==e1 && s2==e2)
               tmp = 0 ;
            else if (s1<e1 && s2<e2)
               tmp = (*s1<*s2) ? -1 : 1 ;
            else
               tmp = (s1<e1) ? 1 : -1 ;
         }

         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;

         type = this->type ;
         if (tmp==0)
            sint = (type==X_GTE || type==X_LTE || type==X_EQUAL) ;
         else if (tmp>0)
            sint = (type==X_GT || type==X_GTE || type==X_DIFF) ;
         else
            sint = (type==X_LT || type==X_LTE || type==X_DIFF) ;

         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'O' ) ;

         return sint ;
      }

      case X_SGT:
      case X_SLT:
      case X_SLTE:
      case X_SGTE:
      case X_SEQUAL:
      case X_SDIFF:
      {  /* string comparison */
         char *s1, *s2, *e1, *e2 ;
         int type ;

         type = this->type ;
         strone = evaluate( TSD, this->p[0], &stmp1 ) ;
         strtwo = evaluate( TSD, this->p[1], &stmp2 ) ;

         s1 = strone->value ;
         s2 = strtwo->value ;
         e1 = s1 + strone->len ;
         e2 = s2 + strtwo->len ;
         /*
          * Strip leading spaces - ignored in comparison
          */
         for(;(s1<e1)&&(isspace(*s1));s1++)
         {
            ;
         }
         for(;(s2<e2)&&(isspace(*s2));s2++) 
         {
            ;
         }
         for (;(s1<e1)&&(s2<e2)&&(*s1==*s2);s1++,s2++)
         {
            ;
         }
         /*
          * Strip trailing spaces - ignored in comparison
          */
         /*  wait a bit! it is not (isspace(*s1)) we should test , but the end of string!     bja
         for (;(e1>s1)&&(isspace(*s1));e1--) ;
         for (;(e2>s2)&&(isspace(*s2));e2--) ;
         */                                                                                /* bja */
         for (;(e1>s1)&&(isspace(*(e1-1)));e1--)                                           /* bja */
         {
            ;
         }
         for (;(e2>s2)&&(isspace(*(e2-1)));e2--)                                           /* bja */
         {
            ;
         }

         if (s1==e1 && s2==e2)
            tmp = 0 ;
         else if (s1<e1 && s2<e2)
            tmp = (*s1<*s2) ? -1 : 1 ;
         else
            tmp = (s1<e1) ? 1 : -1 ;

         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;

         if (tmp==0)
            sint = (type==X_SGTE || type==X_SLTE || type==X_SEQUAL) ;
         else if (tmp>0)
            sint = (type==X_SGT || type==X_SGTE || type==X_SDIFF) ;
         else
            sint = (type==X_SLT || type==X_SLTE || type==X_SDIFF) ;

         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'O' ) ;

         return sint ;
      }

      case X_NGT:
      case X_NLT:
      case X_NLTE:
      case X_NGTE:
      case X_NEQUAL:
      case X_NDIFF:
      {
         /* numeric NOT comparison */
         int type ;
         num_descr *ntmp1, *ntmp2 ;
         num_descr *numone, *numtwo ;

         type = this->type ;

         numone = calcul( TSD, this->p[0], &ntmp1 ) ;
         numtwo = calcul( TSD, this->p[1], &ntmp2 ) ;
         tmp = string_test( TSD, numone, numtwo ) ;

         if (ntmp1)
         {
            FreeTSD( ntmp1->num ) ;
            FreeTSD( ntmp1 ) ;
         }
         if (ntmp2)
         {
            FreeTSD( ntmp2->num ) ;
            FreeTSD( ntmp2 ) ;
         }

         if (tmp==0)
            sint = (type==X_NGTE || type==X_NLTE || type==X_NEQUAL) ;
         else if (tmp>0)
            sint = (type==X_NGT || type==X_NGTE || type==X_NDIFF) ;
         else
            sint = (type==X_NLT || type==X_NLTE || type==X_NDIFF) ;

         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'O' ) ;

         return sint ;
      }

      case X_S_DIFF:
         strone = evaluate( TSD, this->p[0], &stmp1 ) ;
         strtwo = evaluate( TSD, this->p[1], &stmp2 ) ;
         tmp = Str_cmp(strone,strtwo)!=0 ;
         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;
         if (TSD->trace_stat=='I')
            tracebool( TSD, tmp, 'O' ) ;

         return tmp ;


      case X_S_EQUAL:
         strone = evaluate( TSD, this->p[0], &stmp1 ) ;
         strtwo = evaluate( TSD, this->p[1], &stmp2 ) ;
         tmp = Str_cmp(strone,strtwo)==0 ;
         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;
         if (TSD->trace_stat=='I')
            tracebool( TSD, tmp, 'O' ) ;

         return tmp ;

      case X_S_NGT:
      case X_S_NLT:
      {  /* strict string NOT comparison */
         char *s1, *s2, *e1, *e2 ;
         int type ;

         type = this->type ;
         strone = evaluate( TSD, this->p[0], &stmp1 ) ;
         strtwo = evaluate( TSD, this->p[1], &stmp2 ) ;

         s1 = strone->value ;
         s2 = strtwo->value ;
         e1 = s1 + strone->len ;
         e2 = s2 + strtwo->len ;
         /*
          * same compare as non-strict except that leading and trailing spaces
          * are retained for comparison.
          */
         for (;(s1<e1)&&(s2<e2)&&(*s1==*s2);s1++,s2++) ;
         if (s1==e1 && s2==e2)
            tmp = 0 ;
         else if (s1<e1 && s2<e2)
            tmp = (*s1<*s2) ? 1 : -1 ;
         else
            tmp = (s1<e1) ? -1 : 1 ;

         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;

         if (tmp==0)
            sint = 1;
         else if (tmp>0)
            sint = (type==X_S_NGT) ;
         else
            sint = (type==X_S_NLT) ;

         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'O' ) ;

         return sint ;
      }

      case X_S_GT:
      case X_S_GTE:
      case X_S_LT:
      case X_S_LTE:
      {  /* strict string comparison */
         char *s1, *s2, *e1, *e2 ;
         int type ;

         type = this->type ;
         strone = evaluate( TSD, this->p[0], &stmp1 ) ;
         strtwo = evaluate( TSD, this->p[1], &stmp2 ) ;

         s1 = strone->value ;
         s2 = strtwo->value ;
         e1 = s1 + strone->len ;
         e2 = s2 + strtwo->len ;
         /*
          * same compare as non-strict except that leading and trailing spaces
          * are retained for comparison.
          */
         for (;(s1<e1)&&(s2<e2)&&(*s1==*s2);s1++,s2++) ;
         if (s1==e1 && s2==e2)
            tmp = 0 ;
         else if (s1<e1 && s2<e2)
            tmp = (*s1<*s2) ? -1 : 1 ;
         else
            tmp = (s1<e1) ? 1 : -1 ;

         if (stmp1)
            Free_stringTSD( stmp1 ) ;
         if (stmp2)
            Free_stringTSD( stmp2 ) ;

         if (tmp==0)
            sint = (type==X_S_GTE || type==X_S_LTE) ;
         else if (tmp>0)
            sint = (type==X_S_GT || type==X_S_GTE ) ;
         else
            sint = (type==X_S_LT || type==X_S_LTE ) ;

         if (TSD->trace_stat=='I')
            tracebool( TSD, sint, 'O' ) ;

         return sint ;
      }


      default:
         exiterror( ERR_INTERPRETER_FAILURE, 1, __FILE__, __LINE__, "" )  ;
         return 0 ;
   }
}

