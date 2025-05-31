#pragma once
#include <Arduino.h>

class IntervalTimer
{
  uint32_t delay_ms, last_ms;

public:
  /**
   * Nom: IntervalTimer::IntervalTimer
   * Fonction: Constructeur. initialize l'intervale immediatement.
   * Argument(s) réception: le delai attendu
   * Argument(s) de retour: (rien)
   * Modifie/utilise (globale): (rien)
   * Notes: (rien)
   */
  IntervalTimer(uint32_t delay)
      : delay_ms(delay), last_ms(0) {}

  /**
   * Nom: IntervalTimer::is_time
   * Fonction: détermine si l'intervale est écoulée.
   * Argument(s) réception: le temps actuel en millis
   * Argument(s) de retour: si l'intervale s'est écoulée.
   * Modifie/utilise (globale): (rien)
   * Notes: (rien)
   */
  bool is_time(uint32_t now)
  {
    return now > this->last_ms + this->delay_ms;
  }

  /**
   * Nom: IntervalTimer::is_time_mut
   * Fonction: version mutable de is_time_mut
   * Argument(s) réception: le temps actuel en millis
   * Argument(s) de retour: si l'intervale s'est écoulée.
   * Modifie/utilise (globale): (rien)
   * Notes:
   * - doit etre appellée au maximum une fois par loop, après tout
   * usages de is_time (la version non mutable de la fonction)
   */
  bool is_time_mut(uint32_t now)
  {
    if (this->is_time(now))
    {
      this->last_ms = now;
      return true;
    }
    return false;
  }

  /**
   * Nom: IntervalTimer::get_time_left
   * Fonction: détermine le temps restant avant la prochaine intervale.
   * Argument(s) réception: le temps actuel en millis.
   * Argument(s) de retour: le temps restant en millis.
   * Modifie/utilise (globale): (rien)
   * Notes: (rien)
   */
  uint32_t get_time_left(uint32_t now)
  {
    return (this->last_ms + this->delay_ms) - now;
  }
};