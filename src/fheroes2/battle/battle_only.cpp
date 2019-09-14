/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "agg.h"
#include "cursor.h"
#include "settings.h"
#include "gamedefs.h"
#include "battle.h"
#include "world.h"
#include "castle.h"
#include "kingdom.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "heroes_recruits.h"
#include "dialog_selectitems.h"
#include "button.h"
#include "race.h"
#include "text.h"
#include "game.h"
#include "army_bar.h"
#include "battle_only.h"

#define PRIMARY_MAX_VALUE	20

void RedrawPrimarySkillInfo(const Point &, PrimarySkillsBar*, PrimarySkillsBar*); /* heroes_meeting.cpp */

void Battle::ControlInfo::Redraw()
{
    const Sprite & cell = AGG::GetICN(ICN::CELLWIN, 1);
    const Sprite & mark = AGG::GetICN(ICN::CELLWIN, 2);

    cell.Blit(rtLocal.x, rtLocal.y);
    if(result & CONTROL_HUMAN) mark.Blit(rtLocal.x + 3, rtLocal.y + 2);
    Text text("Human", Font::SMALL);
    text.Blit(rtLocal.x + cell.w() + 5, rtLocal.y + 5);

    cell.Blit(rtAI.x, rtAI.y);
    if(result & CONTROL_AI) mark.Blit(rtAI.x + 3, rtAI.y + 2);
    text.Set("AI");
    text.Blit(rtAI.x + cell.w() + 5, rtAI.y + 5);
}

Battle::Only::Only() : hero1(NULL), hero2(NULL), player1(Color::BLUE), player2(Color::NONE),
	army1(NULL), army2(NULL), moraleIndicator1(NULL), moraleIndicator2(NULL),
	luckIndicator1(NULL), luckIndicator2(NULL), primskill_bar1(NULL), primskill_bar2(NULL),
	secskill_bar1(NULL), secskill_bar2(NULL), selectArmy1(NULL), selectArmy2(NULL),
	selectArtifacts1(NULL), selectArtifacts2(NULL), cinfo2(NULL),
	rt1(36, 267, 43, 53), sfb1(rt1, false), rt2(23, 347, 34, 34), sfb2(rt2, false)
{
    player1.SetControl(CONTROL_HUMAN);
    player2.SetControl(CONTROL_AI);

    const Sprite &backSprite = AGG::GetICN(ICN::SWAPWIN, 0);

    backSprite.Blit(rt1, 0, 0, sfb1);
    backSprite.Blit(rt2, 0, 0, sfb2);

    const RGBA gray = RGBA(0xb0, 0xb0, 0xb0);

    sfc1.Set(rt1.w, rt1.h, true);
    sfc1.DrawBorder(gray);

    sfc2.Set(rt2.w, rt2.h, true);
    sfc2.DrawBorder(gray);
}

StreamBase & operator<< (StreamBase & msg, const Battle::Only & b)
{
    return msg <<
	b.hero1->GetID() << *b.hero1 <<
	b.hero2->GetID() << *b.hero2 <<
	b.player1 << b.player2;
}

StreamBase & operator>> (StreamBase & msg, Battle::Only & b)
{
    int id = 0;

    msg >> id;
    b.hero1 = world.GetHeroes(id);
    if(b.hero1)
	msg >> *b.hero1;
    else
       DEBUG(DBG_NETWORK, DBG_WARN, "unknown id");

    msg >> id;
    b.hero2 = world.GetHeroes(id);
    if(b.hero2)
	msg >> *b.hero2;
    else
	DEBUG(DBG_NETWORK, DBG_WARN, "unknown id");

    msg >>
	b.player1 >> b.player2;

    return msg;
}

Recruits Battle::Only::GetHeroesFromStreamBuf(StreamBuf & msg)
{
    Recruits heroes;
    Battle::Only b;
    msg >> b;
    heroes.SetHero1(b.hero1);
    heroes.SetHero2(b.hero2);
    return heroes;
}

bool Battle::Only::ChangeSettings()
{
    Settings & conf = Settings::Get();
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    Dialog::FrameBorder frameborder(Size(640, 480));

    const Point & cur_pt = frameborder.GetArea();

    rtPortrait1 = Rect(cur_pt.x + 93, cur_pt.y + 72, 101, 93);
    rtPortrait2 = Rect(cur_pt.x + 445, cur_pt.y + 72, 101, 93);

    rtAttack1 = Rect(cur_pt.x + 215, cur_pt.y + 50, 33, 33);
    rtAttack2 = Rect(cur_pt.x + 390, cur_pt.y + 50, 33, 33);

    rtDefense1 = Rect(cur_pt.x + 215, cur_pt.y + 83, 33, 33);
    rtDefense2 = Rect(cur_pt.x + 390, cur_pt.y + 83, 33, 33);

    rtPower1 = Rect(cur_pt.x + 215, cur_pt.y + 116, 33, 33);
    rtPower2 = Rect(cur_pt.x + 390, cur_pt.y + 116, 33, 33);

    rtKnowledge1 = Rect(cur_pt.x + 215, cur_pt.y + 149, 33, 33);
    rtKnowledge2 = Rect(cur_pt.x + 390, cur_pt.y + 149, 33, 33);

    if(conf.GameType(Game::TYPE_NETWORK))
    {
	player2.SetColor(Color::RED);

	player1.SetControl(CONTROL_REMOTE);
	player2.SetControl(CONTROL_REMOTE);

	hero2 = world.GetHeroes(Heroes::ZOM);
    }

    hero1 = world.GetHeroes(Heroes::LORDKILBURN);
    hero1->GetSecondarySkills().FillMax(Skill::Secondary());
    army1 = &hero1->GetArmy();

    RedrawBaseInfo(cur_pt);

    UpdateHero1(cur_pt);

    moraleIndicator1->Redraw();
    luckIndicator1->Redraw();
    primskill_bar1->Redraw();
    secskill_bar1->Redraw();
    selectArtifacts1->Redraw();

    selectArmy1 = new ArmyBar(army1, true, false, true);
    selectArmy1->SetColRows(5, 1);
    selectArmy1->SetPos(cur_pt.x + 36, cur_pt.y + 267);
    selectArmy1->SetHSpace(2);
    selectArmy1->Redraw();

    if(hero2)
    {
	hero2->GetSecondarySkills().FillMax(Skill::Secondary());
	UpdateHero2(cur_pt);

	moraleIndicator2->Redraw();
	luckIndicator2->Redraw();
	secskill_bar2->Redraw();
	selectArtifacts2->Redraw();
	selectArmy2->Redraw();
    }

    monsters.GetTroop(0)->Set(Monster::PEASANT, 100);
    army2 = hero2 ? &hero2->GetArmy() : &monsters;

    selectArmy2 = new ArmyBar(army2, true, false, true);
    selectArmy2->SetColRows(5, 1);
    selectArmy2->SetPos(cur_pt.x + 381, cur_pt.y + 267);
    selectArmy2->SetHSpace(2);
    selectArmy2->Redraw();

    bool exit = false;
    bool redraw = false;
    bool result = false;
    bool allow1 = true;
    bool allow2 = true;

    Button buttonStart(cur_pt.x + 280, cur_pt.y + 428, ICN::SYSTEM, 1, 2);
    buttonStart.Draw();

    cursor.Show();
    display.Flip();

    // message loop
    while(!exit && le.HandleEvents())
    {
        buttonStart.isEnable() &&
	    le.MousePressLeft(buttonStart) ? buttonStart.PressDraw() : buttonStart.ReleaseDraw();

	if((buttonStart.isEnable() && le.MouseClickLeft(buttonStart)) ||
	    Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY))
	{
	    result = true;
	    exit = true;
	}
        else
	if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) exit = true;

	if(allow1 && le.MouseClickLeft(rtPortrait1))
	{
	    int hid = Dialog::SelectHeroes(hero1->GetID());
	    if(hero2 && hid == hero2->GetID())
	    {
		Dialog::Message("Error", "Please, select other hero.", Font::BIG, Dialog::OK);
	    }
	    else
	    if(Heroes::UNKNOWN != hid)
	    {
		hero1 = world.GetHeroes(hid);
		if(hero1) hero1->GetSecondarySkills().FillMax(Skill::Secondary());
		UpdateHero1(cur_pt);
		redraw = true;
	    }
	}
	else
	if(allow2 && le.MouseClickLeft(rtPortrait2))
	{
	    int hid = Dialog::SelectHeroes(hero2 ? hero2->GetID() : Heroes::UNKNOWN);
	    if(hid == hero1->GetID())
	    {
		Dialog::Message("Error", "Please, select other hero.", Font::BIG, Dialog::OK);
	    }
	    else
	    if(Heroes::UNKNOWN != hid)
	    {
		hero2 = world.GetHeroes(hid);
		if(hero2) hero2->GetSecondarySkills().FillMax(Skill::Secondary());
		UpdateHero2(cur_pt);
		if(player2.isControlLocal() && NULL == cinfo2)
		    cinfo2 = new ControlInfo(Point(cur_pt.x + 500, cur_pt.y + 425), player2.GetControl());
		redraw = true;
	    }
	}

	if(hero1 && allow1)
	{
	    if(le.MouseClickLeft(rtAttack1))
	    {
		u32 value = hero1->attack;
		if(Dialog::SelectCount("Set Attack Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero1->attack = value;
		    redraw = true;
		}
	    }
	    else
	    if(le.MouseClickLeft(rtDefense1))
	    {
		u32 value = hero1->defense;
		if(Dialog::SelectCount("Set Defense Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero1->defense = value;
		    redraw = true;
		}
	    }
	    else
	    if(le.MouseClickLeft(rtPower1))
	    {
		u32 value = hero1->power;
		if(Dialog::SelectCount("Set Power Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero1->power = value;
		    redraw = true;
		}
	    }
	    else
	    if(le.MouseClickLeft(rtKnowledge1))
	    {
		u32 value = hero1->knowledge;
		if(Dialog::SelectCount("Set Knowledge Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero1->knowledge = value;
		    redraw = true;
		}
	    }
	}

	if(hero2 && allow2)
	{
	    if(le.MouseClickLeft(rtAttack2))
	    {
		u32 value = hero2->attack;
		if(Dialog::SelectCount("Set Attack Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero2->attack = value;
		    redraw = true;
		}
	    }
	    else
	    if(le.MouseClickLeft(rtDefense2))
	    {
		u32 value = hero2->defense;
		if(Dialog::SelectCount("Set Defense Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero2->defense = value;
		    redraw = true;
		}
	    }
	    else
	    if(le.MouseClickLeft(rtPower2))
	    {
		u32 value = hero2->power;
		if(Dialog::SelectCount("Set Power Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero2->power = value;
		    redraw = true;
		}
	    }
	    else
	    if(le.MouseClickLeft(rtKnowledge2))
	    {
		u32 value = hero2->knowledge;
		if(Dialog::SelectCount("Set Knowledge Skill", 0, PRIMARY_MAX_VALUE, value))
		{
		    hero2->knowledge = value;
		    redraw = true;
		}
	    }
	}

	if(allow1 && le.MouseCursor(selectArmy1->GetArea()) &&
	    selectArmy1->QueueEventProcessing())
	{
	    if(selectArtifacts1->isSelected()) selectArtifacts1->ResetSelected();
	    else
	    if(selectArtifacts2 && selectArtifacts2->isSelected()) selectArtifacts2->ResetSelected();

	    if(selectArmy2->isSelected()) selectArmy2->ResetSelected();

	    redraw = true;
	}

	if(allow2 && le.MouseCursor(selectArmy2->GetArea()) &&
	    selectArmy2->QueueEventProcessing())
	{
	    if(selectArtifacts1->isSelected()) selectArtifacts1->ResetSelected();
	    else
	    if(selectArtifacts2 && selectArtifacts2->isSelected()) selectArtifacts2->ResetSelected();

	    if(selectArmy1->isSelected()) selectArmy1->ResetSelected();

	    redraw = true;
	}

	if(allow1 && le.MouseCursor(selectArtifacts1->GetArea()) &&
	    selectArtifacts1->QueueEventProcessing())
	{
	    if(selectArmy1->isSelected()) selectArmy1->ResetSelected();
	    else
	    if(selectArmy2->isSelected()) selectArmy2->ResetSelected();

	    if(selectArtifacts2 && selectArtifacts2->isSelected()) selectArtifacts2->ResetSelected();

	    redraw = true;
	}

	if(allow2 && selectArtifacts2 && le.MouseCursor(selectArtifacts2->GetArea()) &&
	    selectArtifacts2->QueueEventProcessing())
	{
	    if(selectArmy1->isSelected()) selectArmy1->ResetSelected();
	    else
	    if(selectArmy2->isSelected()) selectArmy2->ResetSelected();

	    if(selectArtifacts1->isSelected()) selectArtifacts1->ResetSelected();

	    redraw = true;
	}

	if(hero1 && allow1)
	{
          if(le.MouseCursor(moraleIndicator1->GetArea())) MoraleIndicator::QueueEventProcessing(*moraleIndicator1);
          else
	  if(le.MouseCursor(luckIndicator1->GetArea())) LuckIndicator::QueueEventProcessing(*luckIndicator1);
          else
	  if(le.MouseCursor(primskill_bar1->GetArea()) && primskill_bar1->QueueEventProcessing())
	      redraw = true;
          else
	  if(le.MouseCursor(secskill_bar1->GetArea()) && secskill_bar1->QueueEventProcessing())
	      redraw = true;
	}

	if(hero2 && allow2)
	{
	  if(le.MouseCursor(moraleIndicator2->GetArea())) MoraleIndicator::QueueEventProcessing(*moraleIndicator2);
	  else
	  if(le.MouseCursor(luckIndicator2->GetArea())) LuckIndicator::QueueEventProcessing(*luckIndicator2);
	  else
	  if(le.MouseCursor(primskill_bar2->GetArea()) && primskill_bar2->QueueEventProcessing())
	      redraw = true;
	  else
	  if(le.MouseCursor(secskill_bar2->GetArea()) && secskill_bar2->QueueEventProcessing())
	      redraw = true;
	}

	if(cinfo2 && allow1)
	{
	    if(hero2 && le.MouseClickLeft(cinfo2->rtLocal) && player2.isControlAI())
	    {
		player2.SetControl(CONTROL_HUMAN);
		redraw = true;
	    }
	    else
	    if(le.MouseClickLeft(cinfo2->rtAI) && player2.isControlHuman())
	    {
		player2.SetControl(CONTROL_AI);
		redraw = true;
	    }
	}

	if(redraw || !cursor.isVisible())
	{
	  cursor.Hide();
	  RedrawBaseInfo(cur_pt);
	  moraleIndicator1->Redraw();
	  luckIndicator1->Redraw();
	  secskill_bar1->Redraw();
	  selectArtifacts1->Redraw();
	  selectArmy1->Redraw();
	  if(hero2)
	  {
	    moraleIndicator2->Redraw();
	    luckIndicator2->Redraw();
	    secskill_bar2->Redraw();
	    selectArtifacts2->Redraw();
	  }
	  selectArmy2->Redraw();
	  if(cinfo2) cinfo2->Redraw();
	  buttonStart.Draw();
	  cursor.Show();
	  display.Flip();
	  redraw = false;
	}
    }

    delete moraleIndicator1;
    delete luckIndicator1;
    delete primskill_bar1;
    delete secskill_bar1;
    delete selectArtifacts1;
    delete selectArmy1;

    if(hero2)
    {
      delete moraleIndicator2;
      delete luckIndicator2;
      delete primskill_bar2;
      delete secskill_bar2;
      delete selectArtifacts2;
      delete selectArmy2;
    }

    if(cinfo2) delete cinfo2;

    return result;
}

void Battle::Only::UpdateHero1(const Point & cur_pt)
{
    if(moraleIndicator1)
    {
      delete moraleIndicator1;
      moraleIndicator1 = NULL;
    }

    if(luckIndicator1)
    {
      delete luckIndicator1;
      luckIndicator1 = NULL;
    }

    if(primskill_bar1)
    {
      delete primskill_bar1;
      primskill_bar1 = NULL;
    }

    if(secskill_bar1)
    {
      delete secskill_bar1;
      secskill_bar1 = NULL;
    }

    if(selectArtifacts1)
    {
      delete selectArtifacts1;
      selectArtifacts1 = NULL;
    }

    if(selectArmy1)
    {
      delete selectArmy1;
      selectArmy1 = NULL;
    }

    if(hero1)
    {
      player1.SetColor(Color::BLUE);
      player1.SetRace(hero1->GetRace());

      moraleIndicator1 = new MoraleIndicator(*hero1);
      moraleIndicator1->SetPos(Point(cur_pt.x + 34, cur_pt.y + 75), true);

      luckIndicator1 = new LuckIndicator(*hero1);
      luckIndicator1->SetPos(Point(cur_pt.x + 34, cur_pt.y + 115), true);

      primskill_bar1 = new PrimarySkillsBar(hero1, true);
      primskill_bar1->SetColRows(1, 4);
      primskill_bar1->SetVSpace(-1);
      primskill_bar1->SetTextOff(70, -25);
      primskill_bar1->SetPos(cur_pt.x + 216, cur_pt.y + 51);

      secskill_bar1 = new SecondarySkillsBar(true, true);
      secskill_bar1->SetColRows(8, 1);
      secskill_bar1->SetHSpace(-1);
      secskill_bar1->SetContent(hero1->GetSecondarySkills().ToVector());
      secskill_bar1->SetPos(cur_pt.x + 22, cur_pt.y + 199);

      selectArtifacts1 = new ArtifactsBar(hero1, true, false, true);
      selectArtifacts1->SetColRows(7, 2);
      selectArtifacts1->SetHSpace(2);
      selectArtifacts1->SetVSpace(2);
      selectArtifacts1->SetContent(hero1->GetBagArtifacts());
      selectArtifacts1->SetPos(cur_pt.x + 23, cur_pt.y + 347);

      army1 = &hero1->GetArmy();

      selectArmy1 = new ArmyBar(army1, true, false, true);
      selectArmy1->SetColRows(5, 1);
      selectArmy1->SetPos(cur_pt.x + 36, cur_pt.y + 267);
      selectArmy1->SetHSpace(2);
    }
}

void Battle::Only::UpdateHero2(const Point & cur_pt)
{
    if(moraleIndicator2)
    {
      delete moraleIndicator2;
      moraleIndicator2 = NULL;
    }

    if(luckIndicator2)
    {
      delete luckIndicator2;
      luckIndicator2 = NULL;
    }

    if(primskill_bar2)
    {
      delete primskill_bar2;
      primskill_bar2 = NULL;
    }

    if(secskill_bar2)
    {
      delete secskill_bar2;
      secskill_bar2 = NULL;
    }

    if(selectArtifacts2)
    {
      delete selectArtifacts2;
      selectArtifacts2 = NULL;
    }

    if(selectArmy2)
    {
      delete selectArmy2;
      selectArmy2 = NULL;
    }

    if(hero2)
    {
      player2.SetColor(Color::RED);
      player2.SetRace(hero2->GetRace());

      moraleIndicator2 = new MoraleIndicator(*hero2);
      moraleIndicator2->SetPos(Point(cur_pt.x + 566, cur_pt.y + 75), true);

      luckIndicator2 = new LuckIndicator(*hero2);
      luckIndicator2->SetPos(Point(cur_pt.x + 566, cur_pt.y + 115), true);

      primskill_bar2 = new PrimarySkillsBar(hero2, true);
      primskill_bar2->SetColRows(1, 4);
      primskill_bar2->SetVSpace(-1);
      primskill_bar2->SetTextOff(-70, -25);
      primskill_bar2->SetPos(cur_pt.x + 389, cur_pt.y + 51);

      secskill_bar2 = new SecondarySkillsBar(true, true);
      secskill_bar2->SetColRows(8, 1);
      secskill_bar2->SetHSpace(-1);
      secskill_bar2->SetContent(hero2->GetSecondarySkills().ToVector());
      secskill_bar2->SetPos(cur_pt.x + 353, cur_pt.y + 199);

      selectArtifacts2 = new ArtifactsBar(hero2, true, false, true);
      selectArtifacts2->SetColRows(7, 2);
      selectArtifacts2->SetHSpace(2);
      selectArtifacts2->SetVSpace(2);
      selectArtifacts2->SetContent(hero2->GetBagArtifacts());
      selectArtifacts2->SetPos(cur_pt.x + 367, cur_pt.y + 347);

      army1 = &hero1->GetArmy();

      selectArmy2 = new ArmyBar(army2, true, false, true);
      selectArmy2->SetColRows(5, 1);
      selectArmy2->SetPos(cur_pt.x + 381, cur_pt.y + 267);
      selectArmy2->SetHSpace(2);
    }
}

void Battle::Only::RedrawBaseInfo(const Point & top)
{
    Display & display = Display::Get();

    AGG::GetICN(ICN::SWAPWIN, 0).Blit(top);

    // header
    std::string message = "%{name1} vs %{name2}";

    StringReplace(message, "%{name1}", std::string(Race::String(hero1->GetRace())) + " " + hero1->GetName());
    if(hero2)
	StringReplace(message, "%{name2}", (hero2 ? std::string(Race::String(hero2->GetRace())) + " " + hero2->GetName() : "Monsters"));

    Text text(message, Font::BIG);
    text.Blit(top.x + 320 - text.w() / 2, top.y + 26);

    // portrait
    Surface port1 = hero1->GetPortrait(PORT_BIG);
    if(port1.isValid()) port1.Blit(rtPortrait1.x, rtPortrait1.y, display);

    if(hero2)
    {
	Surface port2 = hero2->GetPortrait(PORT_BIG);
	if(port2.isValid()) port2.Blit(rtPortrait2.x, rtPortrait2.y, display);
    }
    else
    {
      display.FillRect(rtPortrait2, ColorBlack);
      text.Set("N/A", Font::BIG);
      text.Blit(rtPortrait2.x + (rtPortrait2.w - text.w()) / 2, rtPortrait2.y + rtPortrait2.h / 2 - 8);
    }

    // primary skill
    RedrawPrimarySkillInfo(top, primskill_bar1, primskill_bar2);
}

void Battle::Only::StartBattle()
{
    Settings & conf = Settings::Get();

    Players & players = conf.GetPlayers();
    players.Init(player1.GetColor() | player2.GetColor());
    world.InitKingdoms();

    players.SetPlayerRace(player1.GetColor(), player1.GetRace());
    players.SetPlayerRace(player2.GetColor(), player2.GetRace());

    conf.SetCurrentColor(player1.GetColor());

    players.SetPlayerControl(player1.GetColor(), player1.GetControl());
    players.SetPlayerControl(player2.GetColor(), player2.GetControl());

    if(hero1)
    {
	hero1->SetSpellPoints(hero1->GetMaxSpellPoints());
	hero1->Recruit(player1.GetColor(), Point(5, 5));

	if(hero2)
	{
    	    hero2->SetSpellPoints(hero2->GetMaxSpellPoints());
    	    hero2->Recruit(player2.GetColor(), Point(5, 6));
	}

	Battle::Loader(hero1->GetArmy(), (hero2 ? hero2->GetArmy() : monsters), hero1->GetIndex() + 1);
    }
}
