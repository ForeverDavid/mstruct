/*  ObjCryst++ Object-Oriented Crystallographic Library
    (c) 2000-2002 Vincent Favre-Nicolin vincefn@users.sourceforge.net
        2000-2001 University of Geneva (Switzerland)

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
#include <cmath>
#include <typeinfo>
#include <iomanip>
#include <fstream>

#include <stdio.h> //for sprintf()


#include "cctbx/eltbx/xray_scattering.h"
#include "cctbx/eltbx/tiny_pse.h"
#include "cctbx/eltbx/icsd_radii.h"
#include "cctbx/eltbx/henke.h"
#include "cctbx/eltbx/neutron.h"

#include "ObjCryst/ScatteringPower.h"
#include "Quirks/VFNStreamFormat.h"
#include "Quirks/VFNDebug.h"
#include "ObjCryst/Colours.h" 

#ifdef __WX__CRYST__
   #include "wxCryst/wxScatteringPower.h"
#endif

namespace ObjCryst
{

const RefParType *gpRefParTypeScattPow=0;
const RefParType *gpRefParTypeScattPowResonant=0;
const RefParType *gpRefParTypeScattPowTemperature=0;
const RefParType *gpRefParTypeScattPowTemperatureIso=0;
const RefParType *gpRefParTypeScattPowTemperatureAniso=0;
long NiftyStaticGlobalObjectsInitializer_ScatteringPower::mCount=0;
//######################################################################
//
//      SCATTERING POWER
//
//######################################################################
ObjRegistry<ScatteringPower> gScatteringPowerRegistry("Global ScatteringPower Registry");

ScatteringPower::ScatteringPower():mDynPopCorrIndex(0),mBiso(1.0),mIsIsotropic(true),
mMaximumLikelihoodNbGhost(0),mFormalCharge(0.0),mBeta(6)
{
   VFN_DEBUG_MESSAGE("ScatteringPower::ScatteringPower():"<<mName,5)
   gScatteringPowerRegistry.Register(*this);
   this->Init();
   mClockMaster.AddChild(mClock);
   mClockMaster.AddChild(mMaximumLikelihoodParClock);
}
ScatteringPower::ScatteringPower(const ScatteringPower& old):
mDynPopCorrIndex(old.mDynPopCorrIndex),mBiso(old.mBiso),mIsIsotropic(old.mIsIsotropic),
mBeta(old.mBeta),
mFormalCharge(old.mFormalCharge)
{
   VFN_DEBUG_MESSAGE("ScatteringPower::ScatteringPower(&old):"<<mName,5)
   gScatteringPowerRegistry.Register(*this);
   this->Init();
   mMaximumLikelihoodPositionError=old.mMaximumLikelihoodPositionError;
   mMaximumLikelihoodNbGhost=old.mMaximumLikelihoodNbGhost;
   mClockMaster.AddChild(mClock);
   mClockMaster.AddChild(mMaximumLikelihoodParClock);
}
ScatteringPower::~ScatteringPower()
{
   VFN_DEBUG_MESSAGE("ScatteringPower::~ScatteringPower():"<<mName,5)
   gScatteringPowerRegistry.DeRegister(*this);
}

const string& ScatteringPower::GetClassName()const
{
   const static string className="ScatteringPower";
   return className;
}

void ScatteringPower::operator=(const ScatteringPower& rhs)
{
   VFN_DEBUG_MESSAGE("ScatteringPower::operator=():"<<mName,2)
   mDynPopCorrIndex=rhs.mDynPopCorrIndex;
   mBiso=rhs.mBiso;
   mIsIsotropic=rhs.mIsIsotropic;
   mBeta=rhs.mBeta;
}

bool ScatteringPower::IsScatteringFactorAnisotropic()const{return false;}
bool ScatteringPower::IsTemperatureFactorAnisotropic()const{return false;}
bool ScatteringPower::IsResonantScatteringAnisotropic()const{return false;}

const string& ScatteringPower::GetSymbol() const {return this->GetName();}
REAL ScatteringPower::GetBiso() const {return mBiso;}
REAL& ScatteringPower::GetBiso() {mClock.Click();return mBiso;}
void ScatteringPower::SetBiso(const REAL newB) { mClock.Click();mBiso=newB;mIsIsotropic=true;}
REAL ScatteringPower::GetBij(const size_t &i, const size_t &j) const
{
    size_t idx = 0;
    if(i == j)
    {
        idx = i - 1;
    }
    else
    {
        idx = i + j;
    }
    return this->GetBij(idx);
}
REAL ScatteringPower::GetBij(const size_t &idx) const
{
    return mBeta(idx);
}
void ScatteringPower::SetBij(const size_t &i, const size_t &j, const REAL newB)
{
    mClock.Click();
    size_t idx = 0;
    if(i == j)
    {
        idx = i - 1;
    }
    else
    {
        idx = i + j;
    }
    this->SetBij(idx, newB);
}
void ScatteringPower::SetBij(const size_t &idx, const REAL newB)
{
    mIsIsotropic=false;
    mBeta(idx) = newB;
}
bool ScatteringPower::IsIsotropic() const {return mIsIsotropic;}
long ScatteringPower::GetDynPopCorrIndex() const {return mDynPopCorrIndex;}
long ScatteringPower::GetNbScatteringPower()const {return gScatteringPowerRegistry.GetNb();}
const RefinableObjClock& ScatteringPower::GetLastChangeClock()const {return mClock;}

const string& ScatteringPower::GetColourName()const{ return mColourName;}
const float* ScatteringPower::GetColourRGB()const{ return mColourRGB;}
void ScatteringPower::SetColour(const string& colourName)
{
   mColourName=colourName;
   this->InitRGBColour();
}
void ScatteringPower::SetColour(const float r,const float g,const float b)
{
   mColourRGB[0]=r;
   mColourRGB[1]=g;
   mColourRGB[2]=b;
}
void ScatteringPower::GetGeneGroup(const RefinableObj &obj,
                                CrystVector_uint & groupIndex,
                                unsigned int &first) const
{
   // One group for all parameters
   unsigned int index=0;
   VFN_DEBUG_MESSAGE("ScatteringPower::GetGeneGroup()",4)
   for(long i=0;i<obj.GetNbPar();i++)
      for(long j=0;j<this->GetNbPar();j++)
         if(&(obj.GetPar(i)) == &(this->GetPar(j)))
         {
            if(index==0) index=first++;
            groupIndex(i)=index;
         }
}

REAL ScatteringPower::GetMaximumLikelihoodPositionError()const 
{return mMaximumLikelihoodPositionError;}

const RefinableObjClock& ScatteringPower::GetMaximumLikelihoodParClock()const
{return mMaximumLikelihoodParClock;}

void ScatteringPower::SetMaximumLikelihoodPositionError(const REAL mle) 
{
   if(mle!=mMaximumLikelihoodPositionError)
   {
      mMaximumLikelihoodPositionError=mle;
      mMaximumLikelihoodParClock.Click();
   }
}

REAL ScatteringPower::GetMaximumLikelihoodNbGhostAtom()const
{return mMaximumLikelihoodNbGhost;}

void ScatteringPower::SetMaximumLikelihoodNbGhostAtom(const REAL nb)
{
   if(nb!=mMaximumLikelihoodNbGhost)
   {
      mMaximumLikelihoodNbGhost=nb;
      mMaximumLikelihoodParClock.Click();
   }
}

REAL ScatteringPower::GetFormalCharge()const{return mFormalCharge;}
void ScatteringPower::SetFormalCharge(const REAL charge)
{mFormalCharge=charge;}

void ScatteringPower::Init()
{
   VFN_DEBUG_MESSAGE("ScatteringPower::Init():"<<mName,2)
   mColourName="White";
   mMaximumLikelihoodPositionError=0;
   mMaximumLikelihoodNbGhost=0;
   VFN_DEBUG_MESSAGE("ScatteringPower::Init():End",2)
}
void ScatteringPower::InitRGBColour()
{
   VFN_DEBUG_MESSAGE("ScatteringPower::InitRGBColour()",2)
   for(long i=0;;)
   {
      if(gPOVRayColours[i].mName==mColourName)
      {
         mColourRGB[0]=gPOVRayColours[i].mRGB[0];
         mColourRGB[1]=gPOVRayColours[i].mRGB[1];
         mColourRGB[2]=gPOVRayColours[i].mRGB[2];
         break;
      }
      i++;
      if(gPOVRayColours[i].mName=="")
      {//could not find colour !
         cout << "Could not find colour:"<<mColourName<<" for ScatteringPower "<<mName<<endl;
         mColourRGB[0]=1;
         mColourRGB[1]=1;
         mColourRGB[2]=1;
         break;
      }
   }
   VFN_DEBUG_MESSAGE("->RGBColour:"<<mColourName<<mColourRGB[0]<<" "<<mColourRGB[1]<<" "<<mColourRGB[2],2)
}

//######################################################################
//
//      SCATTERING POWER ATOM
//
//######################################################################
ObjRegistry<ScatteringPowerAtom> 
   gScatteringPowerAtomRegistry("Global ScatteringPowerAtom Registry");

ScatteringPowerAtom::ScatteringPowerAtom():
ScatteringPower(),mSymbol(""),mAtomicNumber(0),mpGaussian(0)
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::ScatteringPowerAtom():"<<mName,5)
   gScatteringPowerAtomRegistry.Register(*this);
   this->InitRefParList();
}

ScatteringPowerAtom::ScatteringPowerAtom(const string &name,
                                         const string &symbol,
                                         const REAL bIso):
mpGaussian(0)
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::ScatteringPowerAtom(n,s,B):"<<name,5)
   gScatteringPowerAtomRegistry.Register(*this);
   this->InitRefParList();
   this->Init(name,symbol,bIso);
}

ScatteringPowerAtom::ScatteringPowerAtom(const ScatteringPowerAtom& old):
mpGaussian(0)
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::ScatteringPowerAtom(&old):"<<old.mName,5)
   gScatteringPowerAtomRegistry.Register(*this);
   this->Init(old.GetName(),old.mSymbol,old.mBiso);
   //this->InitRefParList(); //?? :TODO: Check
}

ScatteringPowerAtom::~ScatteringPowerAtom()
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::~ScatteringPowerAtom():"<<mName,5)
   gScatteringPowerAtomRegistry.DeRegister(*this);
   delete mpGaussian;
}

const string& ScatteringPowerAtom::GetClassName() const
{
   const static string className="ScatteringPowerAtom";
   return className;
}

void ScatteringPowerAtom::Init(const string &name,const string &symbol,const REAL bIso)
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::Init(n,s,b)"<<mName,4)
   this->ScatteringPower::Init();
   this->SetName(name);
   mSymbol=symbol;
   mBiso=bIso;
   mIsIsotropic=true;
   if(mpGaussian!=0) delete mpGaussian;
   try
   {
      cctbx::eltbx::xray_scattering::wk1995 wk95t(mSymbol);
      mpGaussian=new cctbx::eltbx::xray_scattering::gaussian(wk95t.fetch());

      this->InitAtNeutronScattCoeffs();

      cctbx::eltbx::tiny_pse::table tpse(mSymbol);
      mAtomicNumber=tpse.atomic_number();

      cctbx::eltbx::icsd_radii::table ticsd(mSymbol);
      mRadius= ticsd.radius();
   }
   catch(cctbx::error)
   {
      cout << "WARNING: could not interpret Symbol name !"<<mSymbol<<endl
           << "         Reverting to H !"<<endl;
      (*fpObjCrystInformUser)("Symbol not understood:"+mSymbol);
      this->Init(name,"H",bIso);
   }
   
   
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::Init():/Name="<<this->GetName() \
      <<" /Symbol="<<mSymbol<<" /Atomic Number=" << mAtomicNumber,4)
   
   mDynPopCorrIndex=mAtomicNumber;

   //Init default atom colours for POVRay
   switch(mAtomicNumber)
   {
      case   1: mColourName="White";           break;   //hydrogen    
      case   2: mColourName="White";           break;   //helium      
      case   3: mColourName="White";           break;   //lithium     
      case   4: mColourName="White";           break;   //beryllium   
      case   5: mColourName="White";           break;   //boron       
      case   6: mColourName="Gray50";          break;   //carbon      
      case   7: mColourName="DarkGreen";           break;   //nitrogen    
      case   8: mColourName="Red";             break;   //oxygen      
      case   9: mColourName="White";           break;   //fluorine    
      case  10: mColourName="White";           break;   //neon        
      case  11: mColourName="White";           break;   //sodium      
      case  12: mColourName="White";           break;   //magnesium   
      case  13: mColourName="White";           break;   //aluminium   
      case  14: mColourName="White";           break;   //silicon     
      case  15: mColourName="White";           break;   //phosphorus  
      case  16: mColourName="Yellow";          break;   //sulphur     
      case  17: mColourName="SeaGreen";        break;   //chlorine    
      case  18: mColourName="White";           break;   //argon       
      case  19: mColourName="White";           break;   //potassium   
      case  20: mColourName="White";           break;   //calcium     
      case  21: mColourName="White";           break;   //scandium    
      case  22: mColourName="White";           break;   //titanium    
      case  23: mColourName="White";           break;   //vanadium    
      case  24: mColourName="Gray20";          break;   //chromium    
      case  25: mColourName="MediumWood";      break;   //manganese   
      case  26: mColourName="Orange";          break;   //iron        
      case  27: mColourName="Pink";            break;   //cobalt      
      case  28: mColourName="Green";           break;   //nickel      
      case  29: mColourName="Copper";          break;   //copper      
      case  30: mColourName="Gray80";          break;   //zinc        
      case  31: mColourName="White";           break;   //gallium     
      case  32: mColourName="White";           break;   //germanium   
      case  33: mColourName="White";           break;   //arsenic     
      case  34: mColourName="Flesh";           break;   //selenium    
      case  35: mColourName="White";           break;   //bromine     
      case  36: mColourName="White";           break;   //krypton     
      case  37: mColourName="White";           break;   //rubidium    
      case  38: mColourName="White";           break;   //strontium   
      case  39: mColourName="White";           break;   //yttrium     
      case  40: mColourName="White";           break;   //zirconium   
      case  41: mColourName="White";           break;   //niobium     
      case  42: mColourName="White";           break;   //molybdenum  
      case  43: mColourName="White";           break;   //technetium  
      case  44: mColourName="White";           break;   //ruthenium   
      case  45: mColourName="White";           break;   //rhodium     
      case  46: mColourName="White";           break;   //palladium   
      case  47: mColourName="Silver";          break;   //silver      
      case  48: mColourName="White";           break;   //cadmium     
      case  49: mColourName="White";           break;   //indium      
      case  50: mColourName="White";           break;   //tin         
      case  51: mColourName="White";           break;   //antimony    
      case  52: mColourName="White";           break;   //tellurium   
      case  53: mColourName="OrangeRed";       break;   //iodine      
      case  54: mColourName="White";           break;   //xenon       
      case  55: mColourName="White";           break;   //caesium     
      case  56: mColourName="White";           break;   //barium      
      case  57: mColourName="White";           break;   //lanthanum   
      case  58: mColourName="White";           break;   //cerium      
      case  59: mColourName="White";           break;   //praseodymium
      case  60: mColourName="CornflowerBlue";  break;   //neodymium   
      case  61: mColourName="White";           break;   //promethium  
      case  62: mColourName="White";           break;   //samarium    
      case  63: mColourName="White";           break;   //europium    
      case  64: mColourName="White";           break;   //gadolinium  
      case  65: mColourName="White";           break;   //terbium     
      case  66: mColourName="White";           break;   //dysprosium  
      case  67: mColourName="White";           break;   //holmium     
      case  68: mColourName="SkyBlue";           break;   //erbium      
      case  69: mColourName="White";           break;   //thulium     
      case  70: mColourName="White";           break;   //ytterbium   
      case  71: mColourName="White";           break;   //lutetium    
      case  72: mColourName="White";           break;   //hafnium     
      case  73: mColourName="Blue";            break;   //tantalum    
      case  74: mColourName="White";           break;   //tungsten    
      case  75: mColourName="White";           break;   //rhenium     
      case  76: mColourName="White";           break;   //osmium      
      case  77: mColourName="ForestGreen";     break;   //iridium     
      case  78: mColourName="White";           break;   //platinum    
      case  79: mColourName="Gold";            break;   //gold        
      case  80: mColourName="VioletRed";       break;   //mercury     
      case  81: mColourName="White";           break;   //thallium    
      case  82: mColourName="White";           break;   //lead        
      case  83: mColourName="White";           break;   //bismuth     
      case  84: mColourName="White";           break;   //polonium    
      case  85: mColourName="White";           break;   //astatine    
      case  86: mColourName="White";           break;   //radon       
      case  87: mColourName="White";           break;   //francium    
      case  88: mColourName="White";           break;   //radium      
      case  89: mColourName="White";           break;   //actinium    
      case  90: mColourName="White";           break;   //thorium     
      case  91: mColourName="White";           break;   //protactinium
      case  92: mColourName="White";           break;   //uranium     
      case  93: mColourName="White";           break;   //neptunium   
      case  94: mColourName="White";           break;   //plutonium   
      case  95: mColourName="White";           break;   //americium   
      case  96: mColourName="White";           break;   //curium      
      case  97: mColourName="White";           break;   //berkelium   
      case  98: mColourName="White";           break;   //californium 
      case  99: mColourName="White";           break;   //einsteinium 
      case 100: mColourName="White";           break;   //fermium     
      case 101: mColourName="White";           break;   //mendelevium 
      case 102: mColourName="White";           break;   //nobelium    
      case 103: mColourName="White";           break;   //lawrencium  
      default : mColourName="White";           break;   
   }
   this->InitRGBColour();

   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::Init(n,s,b):End",3)
}

CrystVector_REAL ScatteringPowerAtom::GetScatteringFactor(const ScatteringData &data,
                                                            const int spgSymPosIndex) const
{
   VFN_DEBUG_MESSAGE("ScatteringPower::GetScatteringFactor(&data):"<<mName,3)
   CrystVector_REAL sf(data.GetNbRefl());
   switch(data.GetRadiationType())
   {
      case(RAD_NEUTRON):
      {
         VFN_DEBUG_MESSAGE("ScatteringPower::GetScatteringFactor():NEUTRON:"<<mName,3)
         sf=mNeutronScattLengthReal;
         break;
      }
      case(RAD_XRAY):
      {
         VFN_DEBUG_MESSAGE("ScatteringPower::GetScatteringFactor():XRAY:"<<mName,3)
         if(mpGaussian!=0)
         {
            const long nb=data.GetSinThetaOverLambda().numElements();
            const REAL *pstol=data.GetSinThetaOverLambda().data();
            for(long i=0;i<nb;i++)
               sf(i)=mpGaussian->at_stol(*pstol++);
         }
         else sf=1.0;//:KLUDGE:  Should never happen
         break;
      }
      case(RAD_ELECTRON):
      {
         VFN_DEBUG_MESSAGE("ScatteringPower::GetScatteringFactor():ELECTRON:"<<mName,3)
         if(mpGaussian!=0)
         {
            const REAL z=this->GetAtomicNumber();
            const long nb=data.GetSinThetaOverLambda().numElements();
            const REAL *pstol=data.GetSinThetaOverLambda().data();
            for(long i=0;i<nb;i++)
               sf(i)=(z-mpGaussian->at_stol(*pstol))/(*pstol * *pstol);
               pstol++;
         }
         else sf=1.0;//:KLUDGE:  Should never happen
         break;
      }
   }
   VFN_DEBUG_MESSAGE("ScatteringPower::GetScatteringFactor(&data):End",3)
   return sf;
}

REAL ScatteringPowerAtom::GetForwardScatteringFactor(const RadiationType type) const
{
   REAL sf;
   switch(type)
   {
      case(RAD_NEUTRON):
      {
         sf=mNeutronScattLengthReal;
         break;
      }
      case(RAD_XRAY):
      {
         if(mpGaussian!=0)
            sf=mpGaussian->at_stol(0);
         else sf=1.0;
         break;
      }
      case(RAD_ELECTRON):
      {
         const REAL z=this->GetAtomicNumber();
         (z-mpGaussian->at_stol(0.0001))/(.0001 * .0001);
      }
   }
   VFN_DEBUG_MESSAGE("ScatteringPower::GetScatteringFactor(&data):End",3)
   return sf;
}

CrystVector_REAL ScatteringPowerAtom::GetTemperatureFactor(const ScatteringData &data,
                                                             const int spgSymPosIndex) const
{
   VFN_DEBUG_MESSAGE("ScatteringPower::GetTemperatureFactor(&data):"<<mName,3)
   CrystVector_REAL sf(data.GetNbRefl());
   if(mIsIsotropic)
   {
      // :NOTE: can't use 'return exp(-mBiso*pow2(diffData.GetSinThetaOverLambda()))'
      //using kcc (OK with gcc)
      CrystVector_REAL stolsq(data.GetNbRefl());
      const CrystVector_REAL stol=data.GetSinThetaOverLambda();
      stolsq=stol;
      stolsq*=stol;
      
      #ifdef __VFN_VECTOR_USE_BLITZ__
         #define SF sf
         #define STOLSQ stolsq
      #else
         #define SF (*ssf)
         #define STOLSQ (*sstolsq)

         REAL *ssf=sf.data();
         const REAL *sstolsq=stolsq.data();

         for(long ii=0;ii<sf.numElements();ii++)
         {
      #endif
      
         SF=exp(-mBiso*STOLSQ);
         
      #ifdef __VFN_VECTOR_USE_BLITZ__

      #else
         ssf++;
         sstolsq++;
         }
      #endif

      #undef SF
      #undef STOLSQ
   }
   else
   {
      const REAL b11=mBeta(0);
      const REAL b22=mBeta(1);
      const REAL b33=mBeta(2);
      const REAL b12=mBeta(3);
      const REAL b13=mBeta(4);
      const REAL b23=mBeta(5);
      
      #ifdef __VFN_VECTOR_USE_BLITZ__
         #define HH data.H()
         #define KK data.K()
         #define LL data.L()
         #define SF sf
      #else
         #define HH (*hh)
         #define KK (*kk)
         #define LL (*ll)
         #define SF (*ssf)

         const REAL *hh=(data.GetH()).data();
         const REAL *kk=(data.GetK()).data();
         const REAL *ll=(data.GetL()).data();
         REAL *ssf=sf.data();

         for(long ii=0;ii<sf.numElements();ii++)
         {
      #endif
   
      SF=   exp( -b11*pow(HH,2)
                 -b22*pow(KK,2)
                 -b33*pow(LL,2)
                 -2*b12*HH*KK
                 -2*b13*HH*LL
                 -2*b23*KK*LL);
                 
      #ifdef __VFN_VECTOR_USE_BLITZ__

      #else
         hh++;
         kk++;
         ll++;
         ssf++;
         }
      #endif

      #undef HH
      #undef KK
      #undef LL
      #undef SF
   }
   return sf;
}

CrystMatrix_REAL ScatteringPowerAtom::
   GetResonantScattFactReal(const ScatteringData &data,
                            const int spgSymPosIndex) const
{
   VFN_DEBUG_MESSAGE("ScatteringPower::GetResonantScattFactReal(&data):"<<mName,3)
   CrystMatrix_REAL fprime(1,1);//:TODO: More than one wavelength
   CrystMatrix_REAL fsecond(1,1);
   switch(data.GetRadiationType())
   {
      case(RAD_NEUTRON):
      {
         fprime=0;
         fsecond=mNeutronScattLengthImag;
         break;
      }
      case(RAD_XRAY):
      {
         try
         {
            cctbx::eltbx::henke::table thenke(mSymbol);
            cctbx::eltbx::fp_fdp f=thenke.at_angstrom(data.GetWavelength()(0));

            if(f.is_valid_fp()) fprime(0)=f.fp();
            else fprime(0)=0;
            if(f.is_valid_fdp()) fsecond(0)=f.fdp();
            else fsecond(0)=0;
         }
         catch(cctbx::error)
         {
            fprime(0)=0;
            fsecond(0)=0;
         }
         break;
      }
      case(RAD_ELECTRON):
      {
         fprime=0;
         fsecond=0;
         break;
      }
   }
   return fprime;
}

CrystMatrix_REAL ScatteringPowerAtom::
   GetResonantScattFactImag(const ScatteringData &data,
                            const int spgSymPosIndex) const
{
   VFN_DEBUG_MESSAGE("ScatteringPower::GetResonantScattFactImag():"<<mName,3)
   CrystMatrix_REAL fprime(1,1);//:TODO: More than one wavelength
   CrystMatrix_REAL fsecond(1,1);
   switch(data.GetRadiationType())
   {
      case(RAD_NEUTRON):
      {
         fprime=0;
         fsecond=mNeutronScattLengthImag;
         break;
      }
      case(RAD_XRAY):
      {
         try
         {
            cctbx::eltbx::henke::table thenke(mSymbol);
            cctbx::eltbx::fp_fdp f=thenke.at_angstrom(data.GetWavelength()(0));

            if(f.is_valid_fp()) fprime(0)=f.fp();
            else fprime(0)=0;
            if(f.is_valid_fdp()) fsecond(0)=f.fdp();
            else fsecond(0)=0;
         }
         catch(cctbx::error)
         {
            fprime(0)=0;
            fsecond(0)=0;
         }
         break;
      }
      case(RAD_ELECTRON):
      {
         fprime=0;
         fsecond=0;
         break;
      }
   }
   return fsecond;
}


void ScatteringPowerAtom::SetSymbol(const string& symbol)
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::SetSymbol():"<<mName,5)
   this->Init(this->GetName(),symbol,this->GetBiso());
}
const string& ScatteringPowerAtom::GetSymbol() const
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::GetSymbol():"<<mName,5)
   return mSymbol;
}

string ScatteringPowerAtom::GetElementName() const
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::GetElementName():"<<mName,2)
   try
   {
      cctbx::eltbx::tiny_pse::table tpse(mSymbol);
      return tpse.name();
   }
   catch(cctbx::error)
   {
      cout << "WARNING: could not interpret Symbol:"<<mSymbol<<endl;
   }
   return "Unknown";
}

int ScatteringPowerAtom::GetAtomicNumber() const {return mAtomicNumber;}
REAL ScatteringPowerAtom::GetRadius() const {return mRadius;}

void ScatteringPowerAtom::Print()const
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::Print()",1)
   cout << "ScatteringPowerAtom ("<<this->GetName()<<","
        << FormatString(this->GetSymbol(),4) << ") :"
        << FormatFloat(this->GetBiso());
   VFN_DEBUG_MESSAGE_SHORT("at "<<this,10)
   cout << endl;
}

void ScatteringPowerAtom::InitAtNeutronScattCoeffs()
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::InitAtNeutronScattCoeffs():"<<mName,3)
   mClock.Click();
   try
   {
      cctbx::eltbx::neutron::neutron_news_1992_table nn92t(mSymbol);
      mNeutronScattLengthReal=nn92t.bound_coh_scatt_length_real();
      mNeutronScattLengthImag=nn92t.bound_coh_scatt_length_imag();
   }
   catch(cctbx::error)
   {
      cout << "WARNING: could not interpret symbol for neutron coeefs:"<<mSymbol<<endl;
   }
   
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::InitAtNeutronScattCoeffs():End",3)
}

void ScatteringPowerAtom::InitRefParList()
{
   VFN_DEBUG_MESSAGE("ScatteringPowerAtom::InitRefParList():"<<mName,5)
   {
      RefinablePar tmp("Biso",&mBiso,0.1,5.,
                        gpRefParTypeScattPowTemperatureIso,REFPAR_DERIV_STEP_ABSOLUTE,
                        true,true,true,false);
      tmp.SetDerivStep(1e-3);
      tmp.SetGlobalOptimStep(.5);
      tmp.AssignClock(mClock);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("ML Error",&mMaximumLikelihoodPositionError,0.,1.,
                        gpRefParTypeScattPow,REFPAR_DERIV_STEP_ABSOLUTE,
                        false,true,true,false);
      tmp.SetDerivStep(1e-4);
      tmp.SetGlobalOptimStep(.001);
      tmp.AssignClock(mMaximumLikelihoodParClock);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("ML-Nb Ghost Atoms",&mMaximumLikelihoodNbGhost,0.,10.,
                        gpRefParTypeScattPow,REFPAR_DERIV_STEP_ABSOLUTE,
                        true,true,true,false);
      tmp.SetDerivStep(1e-3);
      tmp.SetGlobalOptimStep(.05);
      tmp.AssignClock(mMaximumLikelihoodParClock);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("Formal Charge",&mFormalCharge,-10.,10.,
                        gpRefParTypeScattPow,REFPAR_DERIV_STEP_ABSOLUTE,
                        true,true,true,false);
      tmp.SetDerivStep(1e-3);
      tmp.SetGlobalOptimStep(.05);
      tmp.AssignClock(mClock);
      this->AddPar(tmp);
   }
}
#ifdef __WX__CRYST__
WXCrystObjBasic* ScatteringPowerAtom::WXCreate(wxWindow* parent)
{
   //:TODO: Check mpWXCrystObj==0
   mpWXCrystObj=new WXScatteringPowerAtom(parent,this);
   return mpWXCrystObj;
}
#endif

//######################################################################
//
//      SCATTERING COMPONENT
//
//######################################################################
ScatteringComponent::ScatteringComponent():
mX(0),mY(0),mZ(0),mOccupancy(0),mpScattPow(0),mDynPopCorr(0)
{}
bool ScatteringComponent::operator==(const ScatteringComponent& rhs) const
{
   return ((mX==rhs.mX) && (mY==rhs.mY) && (mZ==rhs.mZ) &&
           (mOccupancy==rhs.mOccupancy) && (mpScattPow==rhs.mpScattPow));
}
bool ScatteringComponent::operator!=(const ScatteringComponent& rhs) const
{
   return ((mX!=rhs.mX) || (mY!=rhs.mY) || (mZ!=rhs.mZ) ||
           (mOccupancy!=rhs.mOccupancy) || (mpScattPow!=rhs.mpScattPow));
}
void ScatteringComponent::Print()const
{
   cout <<mX<<" "<<mY<<" "<<mZ<<" "<<mOccupancy<<" "<<mDynPopCorr<<" "<<mpScattPow;
   if(0!=mpScattPow) cout <<" "<<mpScattPow->GetName();
   cout<<endl;
}

//######################################################################
//
//      SCATTERING COMPONENT LIST
//
//######################################################################

ScatteringComponentList::ScatteringComponentList()
{
}

ScatteringComponentList::ScatteringComponentList(const long nbComponent)
{
   mvScattComp.resize(nbComponent);
}

ScatteringComponentList::ScatteringComponentList(const ScatteringComponentList &old):
mvScattComp(old.mvScattComp)
{
}

ScatteringComponentList::~ScatteringComponentList()
{
}
void ScatteringComponentList::Reset()
{
   mvScattComp.clear();
}

const ScatteringComponent& ScatteringComponentList::operator()(const long i) const
{
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator()("<<i<<")",1)
   if(i>=this->GetNbComponent())
   {
      this->Print();
      throw ObjCrystException("ScatteringComponentList::operator()(i)::i>mNbComponent!!");
   }
   if(i<0) throw ObjCrystException("ScatteringComponentList::operator()&(i)::i<0!!");
   return mvScattComp[i];
}

ScatteringComponent& ScatteringComponentList::operator()(const long i)
{
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator()&("<<i<<")",1)
   if(i>=this->GetNbComponent())
   {
      this->Print();
      throw ObjCrystException("ScatteringComponentList::operator()&(i)::i>mNbComponent!!");
   }
   if(i<0) throw ObjCrystException("ScatteringComponentList::operator()&(i):: i<0!!");
   return mvScattComp[i];
}

long ScatteringComponentList::GetNbComponent() const {return mvScattComp.size();}

void ScatteringComponentList::operator=(const ScatteringComponentList &rhs)
{
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator=()",1)
   mvScattComp=rhs.mvScattComp;
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator=():End",0)
}
bool ScatteringComponentList::operator==(const ScatteringComponentList &rhs)const
{
   if(rhs.GetNbComponent() != this->GetNbComponent()) return false;
   for(long i=0;i<this->GetNbComponent();i++)
      if( (*this)(i) != rhs(i) ) return false;
   return true;
}

void ScatteringComponentList::operator+=(const ScatteringComponentList &rhs)
{
   for(long i=0;i<rhs.GetNbComponent();i++) 
      mvScattComp.push_back(rhs(i));
}
void ScatteringComponentList::operator+=(const ScatteringComponent &rhs)
{
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator+=()",1)
   mvScattComp.push_back(rhs);
}

void ScatteringComponentList::operator++()
{
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator++()",1)
   mvScattComp.resize(this->GetNbComponent()+1);
}

void ScatteringComponentList::operator--()
{
   VFN_DEBUG_MESSAGE("ScatteringComponentList::operator--()",1)
   if(this->GetNbComponent()>0)
      mvScattComp.resize(this->GetNbComponent()-1);
}

void ScatteringComponentList::Print()const
{
   VFN_DEBUG_ENTRY("ScatteringComponentList::Print()",5)
   cout<<"Number of Scattering components:"<<this->GetNbComponent()<<endl;
   for(long i=0;i<this->GetNbComponent();i++)
   {
      cout << i<<":";
      (*this)(i).Print();
   }
   VFN_DEBUG_EXIT("ScatteringComponentList::Print()",5)
}

}//namespace
