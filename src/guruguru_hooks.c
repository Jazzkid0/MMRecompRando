#include "modding.h"
#include "global.h"

#include "apcommon.h"

struct EnGuruguru;

#define FLAGS (ACTOR_FLAG_TARGETABLE | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_10)

#define THIS ((EnGuruguru*)thisx)

#define GURU_GURU_LIMB_MAX 0x10

#define LOCATION_GURU_GURU GI_MASK_BREMEN

typedef void (*EnGuruguruActionFunc)(struct EnGuruguru*, PlayState*);

typedef struct EnGuruguru {
    /* 0x000 */ Actor actor;
    /* 0x144 */ SkelAnime skelAnime;
    /* 0x188 */ Vec3s jointTable[GURU_GURU_LIMB_MAX];
    /* 0x1E8 */ Vec3s morphTable[GURU_GURU_LIMB_MAX];
    /* 0x248 */ EnGuruguruActionFunc actionFunc;
    /* 0x24C */ s16 headZRot;
    /* 0x24E */ s16 headXRot;
    /* 0x250 */ UNK_TYPE1 unk250[0x2];
    /* 0x252 */ s16 headZRotTarget;
    /* 0x254 */ s16 headXRotTarget;
    /* 0x256 */ UNK_TYPE1 unk256[0xE];
    /* 0x264 */ s16 unusedTimer; // set to 6 and decremented, but never has any effect
    /* 0x266 */ s16 unk266;
    /* 0x268 */ s16 unk268;
    /* 0x26C */ f32 animEndFrame;
    /* 0x270 */ u8 unk270;
    /* 0x272 */ s16 unk272; // set, but never used
    /* 0x274 */ s16 textIdIndex;
    /* 0x276 */ s16 texIndex;
    /* 0x278 */ ColliderCylinder collider;
} EnGuruguru; // size = 0x2C4

static u16 textIDs[] = { 0x292A, 0x292B, 0x292C, 0x292D, 0x292E, 0x292F, 0x2930, 0x2931,
                         0x2932, 0x2933, 0x2934, 0x2935, 0x2936, 0x294D, 0x294E };

void func_80BC6E10(EnGuruguru* this);
void func_80BC73F4(EnGuruguru* this);
void func_80BC7440(EnGuruguru* this, PlayState* play);
void func_80BC6F14(EnGuruguru* this, PlayState* play);
void EnGuruguru_ChangeAnim(EnGuruguru* this, s32 animIndex);

// New action function for direct item giving
void EnGuruguru_GiveItemDirectly(EnGuruguru* this, PlayState* play);

// Patch the initialization function to set up direct item giving when appropriate
RECOMP_PATCH void func_80BC6E10(EnGuruguru* this) {
    // Check if we should give the item directly (randomizer location not checked and night time)
    if (this->actor.params == 1 && !rando_location_is_checked(LOCATION_GURU_GURU)) {
        // Set up for direct item giving instead of conversation
        this->actionFunc = EnGuruguru_GiveItemDirectly;
        this->headZRotTarget = 0;
        this->unk268 = 1;
        this->unk270 = 0;
        this->textIdIndex = 0;
        this->unk272 = 0;
        return;
    }

    // Original logic for other cases
    EnGuruguru_ChangeAnim(this, 0); // GURU_GURU_ANIM_PLAY_STILL
    this->textIdIndex = 0;
    this->unk270 = 0;
    if (this->actor.params == 0) {
        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_38_10)) {
            this->textIdIndex = 1;
        }
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_38_40)) {
        this->textIdIndex = 2;
    } else {
        this->textIdIndex = 3;
        this->unk270 = 1;
    }
    this->headZRotTarget = 0;
    this->unk268 = 1;
    this->actor.textId = textIDs[this->textIdIndex];
    if (((this->textIdIndex == 0) || (this->textIdIndex == 1)) && CHECK_WEEKEVENTREG(WEEKEVENTREG_77_04)) {
        if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_88_04)) {
            this->actor.textId = 0x295F;
        } else {
            this->actor.textId = 0x2960;
        }
    }
    this->unk272 = 0;
    this->actionFunc = func_80BC6F14;
}

// New action function that directly offers the item without conversation
void EnGuruguru_GiveItemDirectly(EnGuruguru* this, PlayState* play) {
    s16 yaw;
    s16 yawTemp;

    SkelAnime_Update(&this->skelAnime);

    if (rando_location_is_checked(LOCATION_GURU_GURU)) {
        func_80BC6E10(this);
        return;
    }

    yawTemp = this->actor.yawTowardsPlayer - this->actor.world.rot.y;
    yaw = ABS_ALT(yawTemp);

    // Check if player is close enough and facing the right direction
    if (yaw <= 0x2890 && this->actor.xzDistToPlayer <= 60.0f) {
        // Directly offer the item without conversation
        if (Actor_HasParent(&this->actor, play)) {
            // Item was taken, mark location as checked and set up post-item state
            this->actor.parent = NULL;
            SET_WEEKEVENTREG(WEEKEVENTREG_38_40);
            Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_RECEIVED_BREMEN_MASK);
            Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_MET_GURU_GURU);

            // Transition to normal conversation state
            func_80BC6E10(this);
        } else {
            // Offer the item directly
            Actor_OfferGetItem(&this->actor, play, GI_MASK_BREMEN, 60.0f, 60.0f);
        }
    }
}

RECOMP_PATCH void func_80BC7068(EnGuruguru* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->unk268 != 0) {
        SkelAnime_Update(&this->skelAnime);
    } else if (this->unusedTimer == 0) {
        this->unusedTimer = 6;
        if (Message_GetState(&play->msgCtx) != TEXT_STATE_5) {
            if (this->unk266 == 0) {
                if (this->headZRotTarget != 0) {
                    this->headZRotTarget = 0;
                } else {
                    this->headZRotTarget = 5000;
                }
            }
        } else {
            if ((player->transformation == PLAYER_FORM_HUMAN) || (player->transformation == PLAYER_FORM_DEKU)) {
                this->headZRotTarget = 5000;
            } else {
                this->headZRotTarget = 0;
            }
        }
    }
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_5) && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);
        this->headZRotTarget = 0;
        if ((this->textIdIndex == 13) || (this->textIdIndex == 14)) {
            Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_MET_GURU_GURU);
            SET_WEEKEVENTREG(WEEKEVENTREG_79_04);
            func_80BC6E10(this);
            return;
        }
        if (this->actor.params == 0) {
            if (this->actor.textId == 0x295F) {
                SET_WEEKEVENTREG(WEEKEVENTREG_88_04);
            }
            if (this->actor.textId == 0x292A) {
                SET_WEEKEVENTREG(WEEKEVENTREG_38_10);
            }
            Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_MET_GURU_GURU);
            func_80BC6E10(this);
            return;
        }
        if (this->textIdIndex == 11) {
            func_80BC73F4(this);
            return;
        }
        if (this->textIdIndex == 12) {
            SET_WEEKEVENTREG(WEEKEVENTREG_38_40);
            Audio_MuteSeqPlayerBgmSub(false);
            Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_RECEIVED_BREMEN_MASK);
            Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_MET_GURU_GURU);
            func_80BC6E10(this);
            return;
        }
        if (this->textIdIndex >= 3) {
            //if ((this->textIdIndex == 0xA) && (INV_CONTENT(ITEM_MASK_BREMEN) == ITEM_MASK_BREMEN)) {
            if ((this->textIdIndex == 0xA) && rando_location_is_checked(LOCATION_GURU_GURU)) {
                this->textIdIndex = 0xC;
                this->unk268 = 0;
            } else {
                this->textIdIndex++;
                this->unk268++;
                this->unk268 &= 1;
                if (this->textIdIndex == 11) {
                    this->unk268 = 0;
                }
            }
            this->texIndex = 0;
            if (this->textIdIndex == 7) {
                this->texIndex = 1;
            }
            if ((this->unk268 != 0) && (this->textIdIndex >= 7)) {
                this->skelAnime.playSpeed = 2.0f;
                Audio_SetSeqTempoAndFreq(3, 1.18921f, 2);
                Audio_MuteSeqPlayerBgmSub(false);
            } else {
                if (this->skelAnime.playSpeed == 2.0f) {
                    Audio_SetSeqTempoAndFreq(3, 1.0f, 2);
                }
                if (this->unk268 == 0) {
                    Audio_MuteSeqPlayerBgmSub(true);
                } else {
                    Audio_MuteSeqPlayerBgmSub(false);
                }
                this->skelAnime.playSpeed = 1.0f;
            }
            this->unk266 = 1;
            Message_ContinueTextbox(play, textIDs[this->textIdIndex]);
            return;
        }
        Audio_MuteSeqPlayerBgmSub(false);
        Message_BombersNotebookQueueEvent(play, BOMBERS_NOTEBOOK_EVENT_MET_GURU_GURU);
        func_80BC6E10(this);
    }
}
