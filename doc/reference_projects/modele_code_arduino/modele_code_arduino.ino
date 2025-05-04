/*  --- Entête principale -- information sur le programme
 *   
 *  Programme:        (nom du programme)
 *  Date:             (date initiale de création)
 *  Auteur:           (nom, initiale de l'auteur de ce code)
 *  Pltform matériel: (plateforme matérielle et version - ESP32 DOIT KIT V1 - protoTPhys)
 *  Pltform develop : (nom et version de la plateforme de développement - Arduino 2.0.3/1.8.19)
 *  Description:      (brève description de ce qu'accompli ce programme/sketche (+lignes si requis)
 *  Fonctionnalités:  (fonctionnalités principales)
 *  Notes:
 *    
 *  -- Inspirations et credits: --
 *   (faire la liste des codes d'inspiration et ou des crédits de code)
 *
 *  Note: les éléments //*** sont à effacer lors de l'utilisation du modèle, de même que l'exemple de l'historique.
 */
 
/* --- Materiel et composants -------------------------------------------
 * //*** ex: - BMP280 : capteur de température
 * //*** (faire la liste...)
*/


/* --- HISTORIQUE de développement --------------------------------------
 * //*** ex: v0.1.x: version initiale de test 
 * //*** ex:    - détails sur ce qui a été fait
 * //*** ex:    - détails sur ce qui a été expérimenté
 * //*** ex:    - détails...
 * //*** ex: v1.0.x: version 1 de niveau production (détails)
 * //*** ex: v2.x: versions futures et développements
 * //*** Version et historique de developpement (plus bas)
 * //*** Consulter: https://semver.org/lang/fr/ 
 * //*** ex: #define  _VERSION "0.1.1"
 * //*** format x.y.z:
 * //***   x: MAJEUR: niveau principal (0=preuve de concept et essais, 1=1ere prod, ...=subsequente);
 * //***   y: INTERMÉDIAIRE: sous-version;
 * //***   z: CORRECTIF: incrément de la sous-version intermédiaire
 */
//-----------------------------------------------------------------------



//--- Déclaration des librairies (en ordre alpha) -----------------------

//*** Idéalement: inscrire le numéro de version de chaque librairie
//***             inscrire l'adresse URL ou trouver cette librairie
//*** ( ... Mettre ici tous les éléments de cette section ... )

#include <Decodeur.h> // https://github.com/TechnoPhysCAL/TGP_Decodeur  version 1.1.2

//-----------------------------------------------------------------------



//--- Definitions -------------------------------------------------------

//*** Lister ici les broches qui seront utilisées dans ce programme
//*** ex: #define ONE_WIRE_BUS 4   //Ceci est un exemple
//*** ex: #define TEMPERATURE_PRECISION 9   //Ceci est un exemple

#define brocheLecture 3

//-----------------------------------------------------------------------


//--- Declaration des objets --------------------------------------------

//*** ( ... Mettre ici tous les éléments de cette section ... )
Decodeur monDecodeur(&Serial);
//-----------------------------------------------------------------------


//--- Constantes --------------------------------------------------------

//*** ( ... Mettre ici tous les éléments de cette section ... )
const int broche = 4;
//-----------------------------------------------------------------------


//--- Variables globales ------------------------------------------------

//*** ( ... Mettre ici tous les éléments de cette section ... )
int compteurTours = 0;
uint32_t minuterieDel = 0;
//-----------------------------------------------------------------------


//--- Prototypes --------------------------------------------------------

//*** ( ... Mettre ici tous les éléments de cette section ... )

//-----------------------------------------------------------------------


//--- Section des routines specifiques ----------------------------------

//*** ENTETE DE ROUTINE/FONCTION (pour CHACUNE)

/* 
 * Nom: 
 * Fonction: 
 * Argument(s) réception: (rien)
 * Argument(s) de retour: (rien)
 * Modifie/utilise (globale):
 * Notes:  (spécial, source, amélioration, etc)
 * 
 */


 //*** [... autres routines ...]



//-----------------------------------------------------------------------

//*** SETUP et LOOP sont toujours les 2 dernières
//--- Setup et Loop -----------------------------------------------------
void setup() {
  // put your setup code here, to run once:

} //Fin de setup()

void loop() {
  // put your main code here, to run repeatedly:

} //Fin de loop()
//-----------------------------------------------------------------------

/* Version de modèle: B - Yh H23 */