#include "modding.h"
#include "global.h"

#include "apcommon.h"

#define OBJECT_BOJ_LIMB_MAX 0x10

struct EnGinkoMan;

 typedef void (*EnGinkoManActionFunc)(struct EnGinkoMan*, PlayState*);

typedef struct EnGinkoMan {
    /* 0x000 */ Actor actor;
    /* 0x144 */ SkelAnime skelAnime;
    /* 0x188 */ EnGinkoManActionFunc actionFunc;
    /* 0x18C */ Vec3s jointTable[OBJECT_BOJ_LIMB_MAX];
    /* 0x1EC */ Vec3s morphTable[OBJECT_BOJ_LIMB_MAX];
    /* 0x24C */ Vec3s limb15Rot;
    /* 0x252 */ Vec3s limb8Rot;
    /* 0x258 */ s16 curTextId;
    /* 0x25A */ s16 serviceFee;
    /* 0x25C */ s16 choiceDepositWithdrawl;
    /* 0x25E */ s16 isNewAccount;
    /* 0x260 */ s16 isStampChecked;
    /* 0x262 */ s16 previousBankValue;
    /* 0x264 */ s16 animTimer;
} EnGinkoMan; // size = 0x268

#define GINKOMAN_CHOICE_DEPOSIT   0
#define GINKOMAN_CHOICE_WITHDRAWL 1
#define GINKOMAN_CHOICE_CANCEL    2
#define GINKOMAN_CHOICE_RESET     0

#define GINKOMAN_CHOICE_YES  0
#define GINKOMAN_CHOICE_NO   1

#define FLAGS (ACTOR_FLAG_TARGETABLE | ACTOR_FLAG_FRIENDLY)

#define THIS ((EnGinkoMan*)thisx)

void EnGinkoMan_SetupIdle(EnGinkoMan* this);
void EnGinkoMan_SetupDialogue(EnGinkoMan* this);
void EnGinkoMan_SetupBankAward(EnGinkoMan* this);
void EnGinkoMan_SetupBankAward2(EnGinkoMan* this);
void EnGinkoMan_SetupStamp(EnGinkoMan* this);

void EnGinkoMan_Idle(EnGinkoMan* this, PlayState* play);
void EnGinkoMan_BankAward(EnGinkoMan* this, PlayState* play);
void EnGinkoMan_DepositDialogue(EnGinkoMan* this, PlayState* play);
void EnGinkoMan_BankAward2(EnGinkoMan* this, PlayState* play);
void EnGinkoMan_Stamp(EnGinkoMan* this, PlayState* play);
void EnGinkoMan_Dialogue(EnGinkoMan* this, PlayState* play);
void EnGinkoMan_SwitchAnimation(EnGinkoMan* this, PlayState* play);

typedef enum {
    /* 0 */ GINKO_ANIM_LEGSMACKING,
    /* 1 */ GINKO_ANIM_SITTING,
    /* 2 */ GINKO_ANIM_REACHING,
    /* 3 */ GINKO_ANIM_AMAZED,
    /* 4 */ GINKO_ANIM_ADVERTISING, // looking around for customers
    /* 5 */ GINKO_ANIM_MAX
} GinkoAnimation;

extern AnimationHeader object_boj_Anim_0008C0;
extern AnimationHeader object_boj_Anim_0043F0;
extern AnimationHeader object_boj_Anim_004F40;
extern AnimationHeader object_boj_Anim_000AC4;
extern AnimationHeader object_boj_Anim_004A7C;

static AnimationInfo sAnimationInfo[GINKO_ANIM_MAX] = {
    { &object_boj_Anim_0008C0, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_LEGSMACKING
    { &object_boj_Anim_0043F0, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_SITTING
    { &object_boj_Anim_004F40, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -4.0f }, // GINKO_ANIM_REACHING
    { &object_boj_Anim_000AC4, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_AMAZED
    { &object_boj_Anim_004A7C, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_ADVERTISING
};

bool awardChecked;
extern bool Actor_TalkOfferAccepted(Actor* this, GameState* play);

RECOMP_PATCH void EnGinkoMan_Idle(EnGinkoMan* this, PlayState* play) {
    s32 yaw = this->actor.yawTowardsPlayer - this->actor.shape.rot.y;

    EnGinkoMan_SwitchAnimation(this, play);
    if (Actor_TalkOfferAccepted(&this->actor, &play->state)) {
        if (HS_GET_BANK_RUPEES() == 0) {
            if (CHECK_WEEKEVENTREG(WEEKEVENTREG_10_08)) {
                Message_StartTextbox(play, 0x44E, &this->actor);
                this->curTextId = 0x44E; // deposit or exit
            } else {
                Message_StartTextbox(play, 0x44D, &this->actor);
                this->curTextId = 0x44D; // example deposit reward. Leave in for rando to spoil item?
            }
        } else {
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
            Message_StartTextbox(play, 0x468, &this->actor);
            this->curTextId = 0x468; // deposit, withdraw or exit
        }
        EnGinkoMan_SetupDialogue(this);
    } else if (ABS_ALT(yaw) < 0x1555) {
        Actor_OfferTalk(&this->actor, play, 100.0f);
    }
}

// action func: non-input dialogue
RECOMP_PATCH void EnGinkoMan_DepositDialogue(EnGinkoMan* this, PlayState* play) {
    if (!Message_ShouldAdvance(play)) {
        return;
    }

    switch (this->curTextId) {
        case 0x44C:
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
            if (CHECK_WEEKEVENTREG(WEEKEVENTREG_10_08)) {
                Message_StartTextbox(play, 0x44E, &this->actor);
                this->curTextId = 0x44E;
            } else {
                Message_StartTextbox(play, 0x44D, &this->actor);
                this->curTextId = 0x44D;
            }
            break;

        case 0x44D:
            Message_StartTextbox(play, 0x44E, &this->actor);
            this->curTextId = 0x44E;
            break;

        case 0x44F:
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            Message_StartTextbox(play, 0x450, &this->actor);
            this->curTextId = 0x450;
            break;

        case 0x453: // you deposited a tiny amount
        case 0x454: // you deposited a normal amount
        case 0x455: // you deposited a lot
            this->isNewAccount = false;
            if (this->curTextId == 0x453) {
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            }

            play->msgCtx.bankRupees = HS_GET_BANK_RUPEES();
            Message_StartTextbox(play, 0x45A, &this->actor);
            this->curTextId = 0x45A;
            break;

        case 0x456:
        case 0x459:
            Message_StartTextbox(play, 0x44E, &this->actor);
            this->curTextId = 0x44E;
            break;

        case 0x45A:
            if ((HS_GET_BANK_RUPEES() >= 200) && (this->previousBankValue < 200) &&
                !CHECK_WEEKEVENTREG(WEEKEVENTREG_59_40)) {
                SET_WEEKEVENTREG(WEEKEVENTREG_59_40);
                Message_StartTextbox(play, 0x45B, &this->actor);
                this->curTextId = 0x45B;
            } else if ((HS_GET_BANK_RUPEES() >= 500) && (this->previousBankValue < 500) &&
                       !CHECK_WEEKEVENTREG(WEEKEVENTREG_59_80)) {
                SET_WEEKEVENTREG(WEEKEVENTREG_59_80);
                Message_StartTextbox(play, 0x45C, &this->actor);
                this->curTextId = 0x45C;
            } else if (HS_GET_BANK_RUPEES() >= 1000) {
                if ((this->previousBankValue < 1000) && !CHECK_WEEKEVENTREG(WEEKEVENTREG_60_01)) {
                    SET_WEEKEVENTREG(WEEKEVENTREG_60_01);
                    Message_StartTextbox(play, 0x45D, &this->actor);
                    this->curTextId = 0x45D;
                } else if (this->previousBankValue < (s16)HS_GET_BANK_RUPEES()) {
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                    Message_StartTextbox(play, 0x45E, &this->actor);
                    this->curTextId = 0x45E;
                } else {
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                    Message_StartTextbox(play, 0x460, &this->actor);
                    this->curTextId = 0x460;
                }
            } else {
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                Message_StartTextbox(play, 0x460, &this->actor);
                this->curTextId = 0x460;
            }
            break;

        case 0x45B: // given 200 reward
        case 0x45C: // given 500 reward
        case 0x45D: // given 1000 reward
            this->isStampChecked = false;
            Message_CloseTextbox(play);
            EnGinkoMan_SetupBankAward(this);

            if (this->curTextId == 0x45B) {
                if (rando_location_is_checked(LOCATION_BANK_200_REWARD)) {
                    awardChecked = true;
                } else {
                    awardChecked = false;
                }
            } else if (this->curTextId == 0x45C) {
                if (rando_location_is_checked(LOCATION_BANK_500_REWARD)) {
                    awardChecked = true;
                } else {
                    awardChecked = false;
                }
            } else {
                if (rando_location_is_checked(LOCATION_BANK_1000_REWARD)) {
                    awardChecked = true;
                } else {
                    awardChecked = false;
                }
            }

            EnGinkoMan_BankAward(this, play);
            break;

        case 0x461:
        case 0x462:
        case 0x463:
        case 0x464:
        case 0x465:
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            play->msgCtx.bankRupees = HS_GET_BANK_RUPEES();
            Message_StartTextbox(play, 0x45A, &this->actor);
            this->curTextId = 0x45A;
            break;

        case 0x466:
        case 0x467:
            Message_StartTextbox(play, 0x468, &this->actor);
            this->curTextId = 0x468;
            break;

        case 0x469:
            EnGinkoMan_SetupStamp(this); // stamp player
            break;

        case 0x46A:
        case 0x46C:
        case 0x47E:
            if (this->choiceDepositWithdrawl == GINKOMAN_CHOICE_DEPOSIT) {
                if (HS_GET_BANK_RUPEES() >= 5000) {
                    Message_StartTextbox(play, 0x45F, &this->actor);
                    this->curTextId = 0x45F;
                } else if (gSaveContext.save.saveInfo.playerData.rupees == 0) {
                    Message_StartTextbox(play, 0x458, &this->actor);
                    this->curTextId = 0x458;
                } else {
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                    Message_StartTextbox(play, 0x450, &this->actor);
                    this->curTextId = 0x450; // deposit how much [prompt]
                }
            } else { // GINKOMAN_CHOICE_WITHDRAWL
                Message_StartTextbox(play, 0x46E, &this->actor);
                this->curTextId = 0x46E; // withdraw how much
            }

            this->choiceDepositWithdrawl = GINKOMAN_CHOICE_RESET;
            break;

        case 0x46B:
            Message_StartTextbox(play, 0x46E, &this->actor);
            this->curTextId = 0x46E;
            break;

        case 0x46D:
            Message_StartTextbox(play, 0x46E, &this->actor);
            this->curTextId = 0x46E;
            break;

        case 0x470:
            if (Message_ShouldAdvance(play)) {
                Message_CloseTextbox(play);
                this->isStampChecked = false;
                EnGinkoMan_SetupIdle(this); // change to waiting for approach
            }
            break;

        case 0x476:
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
            // fallthrough
        case 0x475:
        case 0x47C:
        case 0x47D:
            Message_StartTextbox(play, 0x468, &this->actor);
            this->curTextId = 0x468;
            break;

        case 0x472:
        case 0x473:
        case 0x474:
            if (HS_GET_BANK_RUPEES() == 0) {
                Message_StartTextbox(play, 0x478, &this->actor);
                this->curTextId = 0x478;
            } else {
                play->msgCtx.bankRupees = HS_GET_BANK_RUPEES();
                Message_StartTextbox(play, 0x45A, &this->actor);
                this->curTextId = 0x45A;
            }
            break;

        case 0x477:
            Message_StartTextbox(play, 0x471, &this->actor);
            this->curTextId = 0x471;
            this->serviceFee = play->msgCtx.unk1206C;
            break;

        case 0x479:
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            Message_StartTextbox(play, 0x450, &this->actor);
            this->curTextId = 0x450; // deposit how much [prompt]
            break;

        default:
            break;
    }
}

RECOMP_PATCH void EnGinkoMan_BankAward(EnGinkoMan* this, PlayState* play) {
    if (Actor_HasParent(&this->actor, play)) {
        // Parent is the player when starting to receive the award
        this->actor.parent = NULL;
        EnGinkoMan_SetupBankAward2(this);
    } else if (this->curTextId == 0x45B) {
        //if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_BANK_WALLET_UPGRADE)) {
        if (awardChecked) {
            Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 500.0f, 100.0f);
        } else {
            Actor_OfferGetItem(&this->actor, play, GI_WALLET_ADULT, 500.0f, 100.0f);
        }
    } else if (this->curTextId == 0x45C) {
        if (awardChecked) {
            Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 500.0f, 100.0f);
        } else {
            Actor_OfferGetItemHook(&this->actor, play, rando_get_item_id(LOCATION_BANK_500_REWARD), LOCATION_BANK_500_REWARD, 500.0f, 100.0f, true, true);
        }
    //} else if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_BANK_HEART_PIECE)) {
    } else if (awardChecked) {
        Actor_OfferGetItem(&this->actor, play, GI_RUPEE_BLUE, 500.0f, 100.0f);
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_HEART_PIECE, 500.0f, 100.0f);
    }
}

RECOMP_PATCH void EnGinkoMan_WaitForDialogueInput(EnGinkoMan* this, PlayState* play) {
    if (!Message_ShouldAdvance(play)) {
        return;
    }

    switch (this->curTextId) {
        case 0x44E: // deposit or exit
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_YES) {
                if (HS_GET_BANK_RUPEES() >= 5000) {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Message_StartTextbox(play, 0x45F, &this->actor);
                    this->curTextId = 0x45F; // bank full, cannot accept more
                } else {
                    if (gSaveContext.save.saveInfo.playerData.rupees > 0) {
                        Audio_PlaySfx_MessageDecide();
                        Message_StartTextbox(play, 0x450, &this->actor);
                        this->curTextId = 0x450; // deposit how much
                    } else {
                        Audio_PlaySfx(NA_SE_SY_ERROR);
                        Message_StartTextbox(play, 0x458, &this->actor);
                        this->curTextId = 0x458; // you're broke
                    }
                }
            } else { // GINKOMAN_CHOICE_NO
                Audio_PlaySfx_MessageCancel();
                Message_StartTextbox(play, 0x460, &this->actor);
                this->curTextId = 0x460; // come again
            }
            break;

        case 0x452: // confirm deposit
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_YES) {
                if (gSaveContext.save.saveInfo.playerData.rupees < play->msgCtx.bankRupeesSelected) {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                    Message_StartTextbox(play, 0x459, &this->actor);
                    this->curTextId = 0x459; // not enough in wallet
                } else {
                    Audio_PlaySfx_MessageDecide();
                    if (HS_GET_BANK_RUPEES() == 0) {
                        this->isNewAccount = true;
                    }

                    Rupees_ChangeBy(-play->msgCtx.bankRupeesSelected);
                    this->previousBankValue = HS_GET_BANK_RUPEES();
                    HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() + play->msgCtx.bankRupeesSelected);
                    play->msgCtx.bankRupees = HS_GET_BANK_RUPEES();
                    Message_StartTextbox(play, 0x45A, &this->actor);
                    this->curTextId = 0x45A; // show rupee total
                }
            } else { // GINKOMAN_CHOICE_NO
                Audio_PlaySfx_MessageCancel();
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                if (HS_GET_BANK_RUPEES() == 0) {
                    Message_StartTextbox(play, 0x44E, &this->actor);
                    this->curTextId = 0x44E; // deposit or exit
                } else {
                    Message_StartTextbox(play, 0x468, &this->actor);
                    this->curTextId = 0x468; // deposit, withdraw or exit
                }
            }
            break;

        case 0x468: // deposit, withdraw or exit
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_CANCEL) {
                Audio_PlaySfx_MessageCancel();
                Message_StartTextbox(play, 0x47C, &this->actor);
                this->curTextId = 0x47C; // cancel withdrawl
            } else {
                Audio_PlaySfx_MessageDecide();
                this->choiceDepositWithdrawl = play->msgCtx.choiceIndex;
                this->isStampChecked = true;
                Message_StartTextbox(play, 0x47E, &this->actor);
                this->curTextId = 0x47E; // deposits total [rupees]
            }
            break;

        case 0x471: // confirm withdrawl
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_YES) {
                if ((s32)HS_GET_BANK_RUPEES() < (play->msgCtx.bankRupeesSelected + this->serviceFee)) {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING); 
                    Message_StartTextbox(play, 0x476, &this->actor);
                    this->curTextId = 0x476; // not enough to withdraw
                } else if (CUR_CAPACITY(UPG_WALLET) <
                           (play->msgCtx.bankRupeesSelected + gSaveContext.save.saveInfo.playerData.rupees)) {
                    // check if wallet is big enough
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Message_StartTextbox(play, 0x475, &this->actor);
                    this->curTextId = 0x475; // not enough wallet space
                } else {
                    Audio_PlaySfx_MessageDecide();
                    this->previousBankValue = HS_GET_BANK_RUPEES();
                    HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() - play->msgCtx.bankRupeesSelected - this->serviceFee);
                    Rupees_ChangeBy(play->msgCtx.bankRupeesSelected);
                    if (HS_GET_BANK_RUPEES() == 0) {
                        Message_StartTextbox(play, 0x478, &this->actor);
                        this->curTextId = 0x478; // emptied account
                    } else {
                        play->msgCtx.bankRupees = HS_GET_BANK_RUPEES();
                        Message_StartTextbox(play, 0x45A, &this->actor);
                        this->curTextId = 0x45A; // show rupee total
                    }
                }
            } else {
                Audio_PlaySfx_MessageCancel();
                Message_StartTextbox(play, 0x468, &this->actor);
                this->curTextId = 0x468; // deposit, withdraw or exit
            }
            break;

        default:
            break;
    }
}

RECOMP_PATCH void EnGinkoMan_WaitForRupeeCount(EnGinkoMan* this, PlayState* play) {
    if (Message_ShouldAdvance(play)) {
        switch (this->curTextId) {
            case 0x450: // deposit how much
                if (play->msgCtx.bankRupeesSelected == 0) {
                    if (HS_GET_BANK_RUPEES() == 0) {
                        Message_StartTextbox(play, 0x44E, &this->actor);
                        this->curTextId = 0x44E; // deposit or exit
                    } else {
                        Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                        Message_StartTextbox(play, 0x468, &this->actor);
                        this->curTextId = 0x468; // deposit, withdraw or exit
                    }
                } else {
                    Message_StartTextbox(play, 0x452, &this->actor);
                    this->curTextId = 0x452; // confirm deposit
                }
                break;

            case 0x46E: // withdraw how much
                if (play->msgCtx.bankRupeesSelected == 0) {
                    Message_StartTextbox(play, 0x47C, &this->actor);
                    this->curTextId = 0x47C; // cancel withdrawl
                } else if (gSaveContext.save.isNight == true) {
                    Message_StartTextbox(play, 0x477, &this->actor);
                    this->curTextId = 0x477; // state service charge // leave in for randomised amount?
                } else {
                    Message_StartTextbox(play, 0x471, &this->actor);
                    this->curTextId = 0x471; // confirm withdrawl
                    this->serviceFee = 0;
                }
                break;

            default:
                break;
        }
    }
}

