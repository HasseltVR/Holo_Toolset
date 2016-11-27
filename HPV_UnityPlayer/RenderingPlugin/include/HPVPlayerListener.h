#ifndef HPV_LISTENER_H
#define HPV_LISTENER_H

#define HPV_EVENT_PLAY 0
#define HPV_EVENT_PAUSE 1
#define HPV_EVENT_STOP 2
#define HPV_EVENT_RESUME 3

namespace HPV {
  
  /* --------------------------------------------------------------------------------- */

  class HPVPlayer;

  /* --------------------------------------------------------------------------------- */

  class HPVPlayerEvent {
  public:
    int type;
    HPVPlayer* player;
  };

  /* --------------------------------------------------------------------------------- */

  class HPVPlayerListener {
  public:
    virtual void onHPVPlayerFrame(unsigned char* pixels, size_t nbytes) { }
    virtual void onHPVPlayerEvent(HPVPlayerEvent& ev) {}
  };
 
} /* namespace poly */


#endif
