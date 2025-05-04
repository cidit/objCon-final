/*  --- Entête principale -- information sur le programme
 *   
 *  Programme:        modele_ex_applic.ino
 *  Date:             22 février 2023
 *  Auteur:           Ali Baba
 *  Pltform matériel: ESP32 DOIT KIT V1
 *  Pltform develop : Arduino 2.2.1
 *  Description:      Programme exemple pour utiliser le modèle de code en TGP
 *  Fonctionnalités:  Ne réalise rien de spécial car seulement utilisé pour démontrer l'utilisation du modèle
 *  Notes:            Aucune
 *    
 *  -- Inspirations et credits: --
 *   https://www.arduino.cc/reference/en/language/functions/math/map/
 *
 */
 
/* --- Materiel et composants -------------------------------------------
 * plaque protoTPhys
 * module GY-49
*/


/* --- HISTORIQUE de développement --------------------------------------
 * v0.1.x: version initiale de test 
 * v1.0.x: ajouté la fonction xyz pour la transformation abc
 * v1.1.x: versions futures et développements
 */

//-----------------------------------------------------------------------



//--- Déclaration des librairies (en ordre alpha) -----------------------

#include <Decodeur.h> // https://github.com/TechnoPhysCAL/TGP_Decodeur  version 1.1.2

//-----------------------------------------------------------------------



//--- Definitions -------------------------------------------------------

#define brocheLecture 3

//-----------------------------------------------------------------------


//--- Declaration des objets --------------------------------------------

Decodeur monDecodeur(&Serial);
//-----------------------------------------------------------------------


//--- Constantes --------------------------------------------------------

const int broche = 4;
//-----------------------------------------------------------------------


//--- Variables globales ------------------------------------------------

int compteurTours = 0;
uint32_t minuterieDel = 0;
//-----------------------------------------------------------------------


//--- Prototypes --------------------------------------------------------

//-----------------------------------------------------------------------


//--- Section des routines specifiques ----------------------------------

/* 
 * Nom: fonction XYZ
 * Fonction: réalise aun calcul simplet
 * Argument(s) réception: auncun
 * Argument(s) de retour: aucun
 * Modifie/utilise (globale): aucun
 * 
 */
void fctXYZ(void) {
  int a=1;
  int b=2;
  int c = a + b;
  return
}

//-----------------------------------------------------------------------

//--- Setup et Loop -----------------------------------------------------
void setup() {
  // put your setup code here, to run once:

} //Fin de setup()

void loop() {
  // put your main code here, to run repeatedly:

} //Fin de loop()
//-----------------------------------------------------------------------

/* Version de modèle: B - Yh H24 */