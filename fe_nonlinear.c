/*
 *  ============================================================================= 
 *  ALADDIN Version 1.0 :
 *       fe_nonlinear.c : Functions Needed for Nonlinear Finite Element Analysis
 *                                                                     
 *  Copyright (C) 1995 by Mark Austin, Xiaoguang Chen, and Wane-Jang Lin
 *  Institute for Systems Research,                                           
 *  University of Maryland, College Park, MD 20742                                   
 *                                                                     
 *  This software is provided "as is" without express or implied warranty.
 *  Permission is granted to use this software for any on any computer system
 *  and to redistribute it freely, subject to the following restrictions:
 * 
 *  1. The authors are not responsible for the consequences of use of
 *     this software, even if they arise from defects in the software.
 *  2. The origin of this software must not be misrepresented, either
 *     by explicit claim or by omission.
 *  3. Altered versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *  4. This notice is to remain intact.
 *                                                                    
 *  Written by: Mark Austin, Xiaoguang Chen, and Wane-Jang Lin      December 1995
 *  ============================================================================= 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "defs.h"
#include "units.h"
#include "matrix.h"
#include "fe_database.h"
#include "symbol.h"
#include "fe_functions.h"
#include "elmt.h"
#include "miscellaneous.h"

extern ARRAY     *array;
extern EFRAME    *frame;

/*
#define DEBUG
*/

static RESPONSE                *RespondBuff;
static int                   *ElmtStateBuff;
static MATER_LOAD_CURVE          *LoadCurve;


/* ============================================================== */
/*        Save Element Actions on Element                         */
/* ============================================================== */
#ifdef __STDC__
void UpdateResponse()
#else
void UpdateResponse()
#endif
{
ELEMENT             *ep;
int             elmt_no;
int                   j;

SYMBOL             *slp;
int    iInPlaneIntegPts;
int  iThicknessIntegPts;
int       iNO_INTEG_pts;

#ifdef DEBUG
       printf("*** Enter UpdateResponse() \n");
#endif

      slp = lookup("InPlaneIntegPts");   /* number of integration pts in plane/surface      */
      if(slp == NULL)
         iInPlaneIntegPts = 2*2;        /* 2x2 as default */
      else
         iInPlaneIntegPts = (int) slp->u.q->value;

      slp = lookup("ThicknessIntegPts"); /* number of integration pts in thickness direction*/
      if(slp == NULL)
         iThicknessIntegPts = 2;        /* 2 as default */
      else
         iThicknessIntegPts = (int) slp->u.q->value;

      iNO_INTEG_pts = iInPlaneIntegPts*iThicknessIntegPts;

      for(elmt_no = 1; elmt_no <= frame->no_elements; elmt_no++) {
          ep = &frame->element[elmt_no-1];
          save_action(array, ep, &frame->eattr[ep->elmt_attr_no -1], elmt_no, iNO_INTEG_pts);
      }

#ifdef DEBUG
       printf("*** Leaving UpdateResponse() \n");
#endif
}

#ifdef __STDC__
void save_action(ARRAY *p, ELEMENT *ep, ELEMENT_ATTR *eap, int elmt_no, int iNO_INTEG_pts)
#else
void save_action(p, ep, eap, elmt_no, iNO_INTEG_pts)
ELEMENT          *ep;
ELEMENT_ATTR    *eap;
ARRAY             *p;
int          elmt_no;
int    iNO_INTEG_pts;

#endif
{
char    *name;
int   i, j, k;
DIMENSIONS *d;

#ifdef DEBUG
       printf("*** Enter save_actions() \n");
#endif

  name = eap->elmt_type;  
  for(i = 1; i <= frame->no_dof; i++ ) {
  for(j = 1; j <= frame->no_nodes_per_elmt; j++) {
       k = frame->no_dof*(j-1) + i;
       ep->rp->Forces->uMatrix.daa[i-1][j-1]
       = RespondBuff[elmt_no-1].Forces->uMatrix.daa[i-1][j-1];
       ep->rp->displ->uMatrix.daa[i-1][j-1] 
       = RespondBuff[elmt_no-1].displ->uMatrix.daa[i-1][j-1];
  }
  }

  ep->rp->max_moment.value = RespondBuff[elmt_no-1].max_moment.value;
  ep->esp->state           = ElmtStateBuff[elmt_no-1];

    /* state, strains parameters */

  switch(ep->esp->state) {
    case 0:   /* ELASTIC state    */
      for(j = 1; j <= iNO_INTEG_pts; j++) {
          ep->rp->effect_pl_strain[j-1] = 0.0;
          for(i = 1; i <= 9; i++) 
              ep->rp->stress->uMatrix.daa[i-1][j-1] 
              = RespondBuff[elmt_no-1].stress->uMatrix.daa[i-1][j-1];
      }
    break;
    case 1:   /* Perfectly plastic or  */
              /* elastic plastic state */
      if(LoadCurve[elmt_no-1].name != NULL) 
         ep->LC_ptr->name = SaveString(LoadCurve[elmt_no-1].name);
      else 
         ep->LC_ptr->name = (char *)NULL;
      

      for(j = 1; j <= iNO_INTEG_pts; j++) {
          ep->rp->effect_pl_strain[j-1]
          += RespondBuff[elmt_no-1].eff_pl_strain_incr[j-1];

          for(i = 1; i <= 9; i++) {
              ep->rp->stress->uMatrix.daa[i-1][j-1] 
              = RespondBuff[elmt_no-1].stress->uMatrix.daa[i-1][j-1];
              ep->rp->strain_pl->uMatrix.daa[i-1][j-1] 
              += RespondBuff[elmt_no-1].strain_pl_incr->uMatrix.daa[i-1][j-1];
          }
          ep->LC_ptr->R[j-1] = LoadCurve[elmt_no-1].R[j-1];
          ep->LC_ptr->H[j-1] = LoadCurve[elmt_no-1].H[j-1];

          for(i = 1; i <= 6; i++) 
             ep->LC_ptr->back_stress[i-1][j-1] 
             = LoadCurve[elmt_no-1].back_stress[i-1][j-1];
      }
    break;
  }
  if(CheckUnits() == ON) {
       switch(UNITS_TYPE) {
         case SI:
           d = DefaultUnits("Pa");
         break;
         case US:
           d = DefaultUnits("psi");
         break;
       }
       for(i = 1; i <= 9; i++) 
           UnitsCopy( &(ep->rp->stress->spRowUnits[i-1]), d );
       free((char *) d->units_name);
       free((char *) d);
  }

#ifdef DEBUG
        printf("******Leaving save_action(): \n");
#endif
}

#ifdef __STDC__
void SetUpRespondBuffer()
#else
void SetUpRespondBuffer()
#endif
{
int                i, j;
int         iNoIntegPts; 

SYMBOL             *slp;
int    iInPlaneIntegPts;
int  iThicknessIntegPts;
int       iNO_INTEG_pts;

ELEMENT_ATTR   *eap;
FIBER_ELMT     *fep;

int	total_fiber_elmt;
int	total_shell_elmt;

#ifdef DEBUG
        printf("*** Eneter SetUpRespondBuffer(): \n");
        printf("***       : no_elments = %d\n", frame->no_elements);
#endif

  total_fiber_elmt = 0;
  total_shell_elmt = 0;

  for(i=1; i<=frame->no_elements; i++)
  {
    eap      = lookup(frame->element[i-1].elmt_attr_name)->u.eap;
    if(eap == NULL)
      FatalError("Elmt_Attribute name not found",(char *)NULL);

    if( !(strcmp(eap->elmt_type, "FIBER")) )
      total_fiber_elmt++;

    if( !(strcmp(eap->elmt_type, "SHELL_4N")) || !(strcmp(eap->elmt_type, "SHELL_8N")) )
      total_shell_elmt++;
  }

  /* Fiber elements exist, setup the needed space for storing load history. */
  /* Including the flags, stresses and strains for each fiber element.      */
  /* The storage space will be static and assigned to frame until updated.  */
  /* The related details are in elmt_fiber.c and be done in elmt_fiber.c    */

  if( total_fiber_elmt != 0 )
    SetUpFiberRespondBuff( total_fiber_elmt, frame );


  /* SHELL_4N or SHELL_8N elements exist, setup the needed space for storing load history. */
  if( total_fiber_elmt != 0 )
  {
    slp = lookup("InPlaneIntegPts");   /* number of integration pts in plane/surface      */
    if(slp == NULL)
       iInPlaneIntegPts = 2*2;        /* 2x2 as default */
    else
       iInPlaneIntegPts = (int) slp->u.q->value;

    slp = lookup("ThicknessIntegPts"); /* number of integration pts in thickness direction*/
    if(slp == NULL)
       iThicknessIntegPts = 2;        /* 2 as default */
    else
       iThicknessIntegPts = (int) slp->u.q->value;

    iNoIntegPts = iInPlaneIntegPts*iThicknessIntegPts;


    RespondBuff   = (RESPONSE *) MyCalloc(frame->no_elements, sizeof(RESPONSE)); 
    ElmtStateBuff = (int *)      MyCalloc(frame->no_elements, sizeof(int)); 
    LoadCurve     = (MATER_LOAD_CURVE *) MyCalloc(frame->no_elements,
                                           sizeof(MATER_LOAD_CURVE)); 

    for(i = 1; i <= frame->no_elements; i++) {
        RespondBuff[i-1].Forces 
        = MatrixAllocIndirect("NodalForce",DOUBLE_ARRAY,6,frame->no_nodes);
        RespondBuff[i-1].displ
        = MatrixAllocIndirect("NodalDispl",DOUBLE_ARRAY,6,frame->no_nodes);
        RespondBuff[i-1].stress
        = MatrixAllocIndirect("Stress",DOUBLE_ARRAY,9,iNoIntegPts);
        RespondBuff[i-1].strain_pl
        = MatrixAllocIndirect("PlasticStrain",DOUBLE_ARRAY,9,iNoIntegPts);
        RespondBuff[i-1].strain_pl_incr
        = MatrixAllocIndirect("PlasticStrainIncr",DOUBLE_ARRAY,9,iNoIntegPts);
        RespondBuff[i-1].effect_pl_strain
        = (double *) MyCalloc(iNoIntegPts, sizeof(double));
        RespondBuff[i-1].eff_pl_strain_incr 
        = (double *) MyCalloc(iNoIntegPts, sizeof(double));
        LoadCurve[i-1].name = (char *) MyCalloc(1,sizeof(char));
        LoadCurve[i-1].R    = (double *) MyCalloc(iNoIntegPts, sizeof(double));
        LoadCurve[i-1].H    = (double *) MyCalloc(iNoIntegPts, sizeof(double));
        LoadCurve[i-1].back_stress 
        = MatrixAllocIndirectDouble(6, iNoIntegPts);
    }
  }  /* end of total_shell_elmt != 0, SHELL_4N or SHELL_8N elements exist */

#ifdef DEBUG
        printf("******Leaving SetUpRespondBuffer(): \n");
#endif
}

#ifdef __STDC__
void SaveRespondBuffer(ARRAY *p, int integ_pt) 
#else
void SaveRespondBuffer(p, integ_pt) 
ARRAY     *p;
int integ_pt;
#endif
{
int i, j, k, kk;
      
#ifdef DEBUG
       printf("*** Enter SaveRespondBuffer(): \n");
#endif

    kk = integ_pt;
    ElmtStateBuff[p->elmt_no-1] = p->elmt_state;

    if(p->LC_ptr->name != NULL) 
       LoadCurve[p->elmt_no-1].name = SaveString(p->LC_ptr->name);
     else
       LoadCurve[p->elmt_no-1].name = NULL;
    
    LoadCurve[p->elmt_no-1].R[kk-1] = p->LC_ptr->R[kk-1];
    LoadCurve[p->elmt_no-1].H[kk-1] = p->LC_ptr->H[kk-1];
   

    for(i = 1; i <= 6; i++)
        LoadCurve[p->elmt_no-1].back_stress[i-1][kk-1]
        = p->LC_ptr->back_stress[i-1][kk-1];

    RespondBuff[p->elmt_no-1].effect_pl_strain[kk-1]
    = p->effect_pl_strain[kk-1];

    RespondBuff[p->elmt_no-1].eff_pl_strain_incr[kk-1]
    = p->eff_pl_strain_incr[kk-1];

    for(i = 1; i <= p->dof_per_node; i++) {
        RespondBuff[p->elmt_no-1].stress->uMatrix.daa[i-1][kk-1]
        = p->stress->uMatrix.daa[i-1][kk-1];
        RespondBuff[p->elmt_no-1].strain_pl->uMatrix.daa[i-1][kk-1]
        = p->strain_pl->uMatrix.daa[i-1][kk-1];
        RespondBuff[p->elmt_no-1].strain_pl_incr->uMatrix.daa[i-1][kk-1]
        = p->strain_pl_incr->uMatrix.daa[i-1][kk-1];
    }

    for(i = 1; i <= p->dof_per_node; i++ ) {
    for(j = 1; j <= p->nodes_per_elmt; j++) {
        k = p->dof_per_node*(j-1) + i;
        RespondBuff[p->elmt_no-1].Forces->uMatrix.daa[i-1][j-1] = p->nodal_loads[k-1].value;
        RespondBuff[p->elmt_no-1].displ->uMatrix.daa[i-1][j-1] = p->displ->uMatrix.daa[i-1][j-1];
    }
    }

  if((p->elmt_type != NULL) && !strcmp(p->elmt_type, "FRAME_2D"))
      RespondBuff[p->elmt_no-1].max_moment.value 
      = MAX( p->nodal_loads[2].value, p->nodal_loads[5].value);
  if((p->elmt_type != NULL) && !strcmp(p->elmt_type, "FRAME_3D"))
      RespondBuff[p->elmt_no-1].max_moment.value
      = MAX( p->nodal_loads[5].value, p->nodal_loads[11].value);

#ifdef DEBUG
        printf("******Leaving SaveRespondBuffer(): \n");
#endif
}
