/***************************************************************************
  file : $URL: file:///develop/SVNrepository/frepple/trunk/src/model/operation_plan.cpp $
  version : $LastChangedRevision$  $LastChangedBy$
  date : $LastChangedDate$
  email : jdetaeye@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * Copyright (C) 2006 by Johan De Taeye                                    *
 *                                                                         *
 * This library is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation; either version 2.1 of the License, or  *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This library is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with this library; if not, write to the Free Software     *
 * Foundation Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA *
 *                                                                         *
 ***************************************************************************/


#include "frepple/model.h"

namespace frepple
{


unsigned long OperationPlan::counter = 1;


void OperationPlan::setChanged(bool b)
{
  if (owner)
    owner->setChanged(b);
  else
  {
    oper->setChanged(b);
    if (lt) lt->setChanged();
  }
}


OperationPlan* OperationPlan::createOperationPlan(const Attributes* atts)
{
	// Pick up the action attribute
  Action action = MetaData::decodeAction(atts);

	// Decode the attributes
	char* opname =
	 XMLString::transcode(atts->getValue(Tags::tag_operation.getXMLCharacters()));

	// Decode the operationplan identifier
  unsigned long id = 0;
  char* idfier =
  	XMLString::transcode(atts->getValue(Tags::tag_id.getXMLCharacters()));
	if (idfier) id = atol(idfier);

  // If an ID is specified, we look up this operation plan
  OperationPlan* opplan = NULL;
  if (idfier)
  {
    opplan = OperationPlan::findId(id);
    if (opplan && opname 
      && strcmp(opplan->getOperation()->getName().c_str(),opname))
    {
      // Previous and current operations don't match.
      ostringstream ch;
      ch << "Operationplan id " << id
        << " defined multiple times with different operations: '"
        << opplan->getOperation() << "' & '" << opname << "'";
			XMLString::release(&opname);
    	XMLString::release(&idfier);
      throw DataException(ch.str());
    }
  }
	XMLString::release(&idfier);

	// Execute the proper action
	switch (action)
	{
		case REMOVE:
			XMLString::release(&opname);
			if (opplan)
      {
        // Send out the notification to subscribers
        LockManager::getManager().obtainWriteLock(opplan);
        if (opplan->getType().raiseEvent(opplan, SIG_REMOVE))
          // Delete it
          delete opplan;
        else
        {
          // The callbacks disallowed the deletion!
          ostringstream ch;
          ch << "Can't delete operationplan with id " << id;
          throw DataException(ch.str());
        }
      }
			else
      {
        ostringstream ch;
        ch << "Can't find operationplan with identifier " 
          << id << " for removal";
        throw DataException(ch.str());
      }
			return NULL;
		case ADD:
			if (opplan)
			{
        ostringstream ch;
        ch << "Operationplan with identifier " << id
				  << " already exists and can't be added again";
				XMLString::release(&opname);
        throw DataException(ch.str());
			}
			if (!opname)
			{
				XMLString::release(&opname);
        throw DataException
          ("Operation name missing for creating an operationplan");
			}
			break;
		case CHANGE:
			if (!opplan)
			{
				ostringstream ch;
        ch << "Operationplan with identifier " << id << " doesn't exist";
				XMLString::release(&opname);
				throw DataException(ch.str());
			}
			break;
		case ADD_CHANGE: ;
	}

	// Return the existing operationplan
	if (opplan)
	{
		XMLString::release(&opname);
    LockManager::getManager().obtainWriteLock(opplan);
		return opplan;
	}

	// Create a new operation plan
  Operation* oper = Operation::find(opname);
	if (!oper)
  {
  	// Can't create operationplan because the operation doesn't exist
    string s(opname);
		XMLString::release(&opname);
    throw DataException("Operation '" + s + "' doesn't exist");
  }
	else
	{
		// Create an operationplan
	  XMLString::release(&opname);
		opplan = oper->createOperationPlan(0.0,0L,0L,NULL,false,NULL,id,false);
    LockManager::getManager().obtainWriteLock(opplan);
    if (!opplan->getType().raiseEvent(opplan, SIG_ADD))
    {
      LockManager::getManager().releaseWriteLock(opplan);
      delete opplan;
      throw DataException("Can't create operationplan");
    }
    return opplan;
	}
}


OperationPlan* OperationPlan::findId(unsigned long l)
{
  // We are garantueed that there are no operationplans that have an id higher
  // than the current counter. This is garantueed by the initialize() method.
  if (l > counter) return NULL;

  // Loop through all operationplans
  for (OperationPlan::iterator i = begin(); i != end(); ++i)
    if (i->id == l) return *i;

  // This ID was not found
  return NULL;
}


void OperationPlan::initialize()
{
  // At least a valid operation pointer must exist
  if (!oper) throw LogicException("Initializing an invalid operationplan");

  // Avoid zero quantity on top-operationplans
  if (getQuantity() <= 0.0 && !owner)
  {
    delete this;
    return;
  }

  // Create unique identifier
  // Having an identifier assigned is an important flag.
  // Only operation plans with an id :
  //   - can be linked in the global operation plan list.
  //   - can have problems (this results from the previous point).
  //   - can be linked with a demand.
  // These properties allow us to delete operation plans without an id faster.
  if (id)
  {
    // An identifier was read in from input
		if (id < counter)
    {
      // The assigned id potentially clashes with an existing operationplan.
      // Check whether it clashes with existing operationplans
      OperationPlan* opplan = findId(id);
      if (opplan && opplan->getOperation()!=oper)
      {
        delete this;
        throw RuntimeException("Duplicated operplanid");
      }
    }
    else
      // The new operationplan definately doesn't clash with existing id's.
      // The counter need updating to garantuee that counter is always
      // a safe starting point for tagging new operationplans.
      counter = id+1;
  }
  else
    // Fresh operationplan with blank id
    id = counter++;

  // Insert into the list of operationplans.  
  // @todo insert such that sorting is more stable!
  next = oper->opplan;
  if (next) next->prev = this;
  oper->opplan = this;

  // If we used the lazy creator, the flow- and loadplans have not been
  // created yet. We do it now...
  createFlowLoads();

  // Extra registration step if this is a delivery operation
  if (lt && lt->getDeliveryOperation() == oper)
    lt->addDelivery(this);

  // Mark the operation to detect its problems
  // Note that a single operationplan thus retriggers the problem computation
  // for all operationplans of this operation. For models with 1) a large
  // number of operationplans per operation and 2) very frequent problem
  // detection, this could constitute a scalability problem. This combination
  // is expected to be unusual and rare, justifying this design choice.
  oper->setChanged();

  // Check the validity of what we have done
  assert(check());
}


void OperationPlan::createFlowLoads()
{
  // Has been initialized already, it seems
  if (!(flowplans.empty() && LoadPlans.empty())) return;

  // Create loadplans
  for(Operation::loadlist::const_iterator g=oper->getLoads().begin();
      g!=oper->getLoads().end(); ++g)
    new LoadPlan(this, *g);

  // Create flowplans
  for(Operation::flowlist::const_iterator h=oper->getFlows().begin();
      h!=oper->getFlows().end(); ++h)
    new FlowPlan(this, *h);
}


OperationPlan::~OperationPlan()
{
  // Delete the flowplans
  for(slist<FlowPlan*>::iterator e = flowplans.begin();
      e != flowplans.end(); ++e)
    delete *e;

  // Delete the loadplans
  for(slist<LoadPlan*>::iterator f = LoadPlans.begin();
      f != LoadPlans.end(); ++f)
    delete *f;

  // Delete also the owner
  if (owner)
  {
    OperationPlan *o = owner;
    setOwner(NULL);
    delete o;
  }

  // The following actions are only required for registered operation plans.
  // Only those are linked in the list and can have problems: see the
  // documentation in the initialize() method.
  if (getIdentifier())
  {
    // Delete from the list of deliveries
    if (lt) lt->removeDelivery(this);

    // Delete from the operationplan list
    if (prev) prev->next = next;
    else oper->opplan = next; // First opplan in the list of this operation
    if (next) next->prev = prev;
  }
}


void OperationPlan::setOwner(OperationPlan* o)
{
  // Special case: the same owner is set twice
  if (owner == o) return;
  // Erase the previous owner if there is one
  if (owner) owner->eraseSubOperationPlan(this);
  // Set new owner
  owner = o;
  // Register with the new owner
  if (owner) owner->addSubOperationPlan(this);
}


void OperationPlan::setStart (Date d)
{
  if (getLocked()) return;
  oper->setOperationPlanParameters(this,quantity,d,0L);

  // Check if update has been allowed
  if (runupdate) update();
  else setChanged();
}


void OperationPlan::setEnd (Date d)
{
  if (getLocked()) return;
  oper->setOperationPlanParameters(this,quantity,0L,d);

  // Check if update has been allowed
  if (runupdate) update();
  else setChanged();
}


void OperationPlan::setQuantity (float f, bool roundDown)
{
  // No impact on locked operationplans
  if (getLocked()) return;

  // Invalid operationplan: the quantity must be >= 0.
  if (f < 0)
    throw DataException("Operationplans can't have negative quantities");

  // Setting a quantity is only allowed on a top operationplan
  if (owner)
  {
    owner->setQuantity(f,roundDown);
    return;
  }

  // Compute the correct size for the operationplan
  if (getOperation()->getSizeMinimum()>0.0f
      && f < getOperation()->getSizeMinimum())
  {
    if (roundDown)
    {
      // Smaller than the minimum quantity, rounding down means... nothing
      quantity = 0.0f;
      // Update the flow and loadplans, and mark for problem detection
      if (runupdate) update();
      else setChanged();
      return;
    }
    f = getOperation()->getSizeMinimum();
  }
  if (getOperation()->getSizeMultiple()>0.0f)
  {
    int mult = (int) (f / getOperation()->getSizeMultiple()
                      + (roundDown ? 0.0f : 0.999999f));
    quantity = mult * getOperation()->getSizeMultiple();
  }
  else
    quantity = f;

  // Update the flow and loadplans, and mark for problem detection
  if (runupdate) update();
  else setChanged();
}


// @todo Investigate the interactions Flpln & oppln setEnd(getDates().getEnd());
void OperationPlan::resizeFlowLoadPlans()
{
  // Update all flowplans
  for (slist<FlowPlan*>::iterator ee = flowplans.begin();
       ee != flowplans.end(); ++ee)
    (*ee)->update();

  // Update all loadplans
  for (slist<LoadPlan*>::iterator e = LoadPlans.begin();
       e != LoadPlans.end(); ++e)
    (*e)->update();

  // Allow the operation length to be changed now that the quantity has changed
  // Note that we assume that the end date remains fixed. This assumption makes
  // sense if the operationplan was created to satisfy a demand.
  // It is not valid though when the purpose of the operationplan was to push 
  // some material downstream.

  // Notify the demand of the changed delivery
  if (lt) lt->setChanged();
}


void OperationPlan::update()
{
  // Update the flow and loadplans
  resizeFlowLoadPlans();

  // Notify the owner operation_plan
  if (owner) owner->update();

  // Mark as changed
  setChanged();
}


void OperationPlan::deleteOperationPlans(Operation* o, bool deleteLockedOpplans)
{
  if (!o) return;
  for (OperationPlan *opplan = o->opplan; opplan; )
  {
    OperationPlan *tmp = opplan;
    opplan = opplan->next;
    // Note that the deletion of the operationplan also updates the opplan list
    if (deleteLockedOpplans || !tmp->getLocked()) delete tmp;
  }
}


void OperationPlan::writeElement(XMLOutput *o, const XMLtag& tag, mode m) const
{
  // Don't export operationplans of hidden operations
  if (oper->getHidden()) return;

  // Writing a reference
  if (m == REFERENCE)
  {
    o->writeElement
      (tag, Tags::tag_id, id, Tags::tag_operation, oper->getName());
    return;
  }

  if (m != NOHEADER)
    o->BeginObject(tag, Tags::tag_id, id, Tags::tag_operation,oper->getName());

  // The demand reference is only valid for delivery operation_plans,
  // and it should only be written if this tag is not being written
  // as part of a demand+delivery tag.
  if (lt && !dynamic_cast<Demand*>(o->getPreviousObject()))
    o->writeElement(Tags::tag_demand, lt);

  o->writeElement(Tags::tag_start, dates.getStart());
  o->writeElement(Tags::tag_end, dates.getEnd());
  o->writeElement(Tags::tag_quantity, quantity);
  if (getLocked()) o->writeElement (Tags::tag_locked, getLocked());
  if (epst) o->writeElement(Tags::tag_epst, epst);
  if (lpst) o->writeElement(Tags::tag_lpst, lpst);
  o->writeElement(Tags::tag_owner, owner);

  o->EndObject(tag);
}


void OperationPlan::beginElement (XMLInput& pIn, XMLElement& pElement)
{
  if (pElement.isA (Tags::tag_demand))
    pIn.readto( MetaCategory::ControllerString<Demand>(Demand::metadata,pIn.getAttributes()) );
  else if (pElement.isA(Tags::tag_owner))
    pIn.readto(createOperationPlan(pIn.getAttributes()));
}


void OperationPlan::endElement (XMLInput& pIn, XMLElement& pElement)
{
  // Note that the fields have been ordered more or less in the order
  // of their expected frequency.
  // Note that id and operation are handled already during the
  // operationplan creation. They don't need to be handled here...
  if (pElement.isA(Tags::tag_quantity))
    pElement >> quantity;
  else if (pElement.isA(Tags::tag_start))
    dates.setStart(pElement.getDate());
  else if (pElement.isA(Tags::tag_end))
    dates.setEnd(pElement.getDate());
  else if (pElement.isA(Tags::tag_owner) && !pIn.isObjectEnd())
  {
    OperationPlan* o = dynamic_cast<OperationPlan*>(pIn.getPreviousObject());
    if (o) setOwner(o);
  }
  else if (pIn.isObjectEnd() && flowplans.empty() && LoadPlans.empty())
  {
    // Note: additional checks for empty flowplans and loadplans is to
    // seperate changes from new ops?
    setAllowUpdates(true);
    initialize();
  }
  else if (pElement.isA (Tags::tag_demand))
  {
    Demand * d = dynamic_cast<Demand*>(pIn.getPreviousObject());
    if (d) d->addDelivery(this);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pElement.isA(Tags::tag_locked))
    setLocked(pElement.getBool());
  else if (pElement.isA(Tags::tag_epst))
    pElement >> epst;
  else if (pElement.isA(Tags::tag_lpst))
    pElement >> lpst;
}


void OperationPlan::setDemand(Demand* l)
{
  // No change
  if (l==lt) return;

  // Unregister from previous lot
  if (lt) lt->removeDelivery(this);

  // Register the new lot and mark it changed
  lt = l;
  if (l) l->setChanged();
}


bool OperationPlan::check()
{
  bool okay = true;

  // Check all flowplans
  for (slist<FlowPlan*>::iterator ee = flowplans.begin();
       ee != flowplans.end(); ++ee)
    okay &= (*ee)->check();

  // Check all loadplans
  for (slist<LoadPlan*>::iterator e = LoadPlans.begin();
       e != LoadPlans.end(); ++e)
    okay &= (*e)->check();

  return okay;
}


void OperationPlanRouting::addSubOperationPlan(OperationPlan* o)
{
  // The sub opplan must point to this operationplan
  assert(o->getOwner() == this);

  // Add in the list
  step_opplans.push_front(o);

  // Update the top operationplan
  setStartAndEnd(
    step_opplans.front()->getDates().getStart(),
    step_opplans.back()->getDates().getEnd()
  );

  // Update if allowed
  if (runupdate) update();
  else setChanged();
}


OperationPlanRouting::~OperationPlanRouting()
{
  // Delete all children
  for (list<OperationPlan*>::iterator i = step_opplans.begin();
       i != step_opplans.end(); ++i)
  {
    // We need to set the owner to NULL first to prevent the sub-operationplan
    // from RE-deleting its parent.
    (*i)->owner = NULL;
    delete (*i);
  }
  step_opplans.clear();
}


void OperationPlanRouting::setQuantity (float f, bool roundDown)
{
  // First the normal resizing
  OperationPlan::setQuantity(f,roundDown);

  // Apply the same size also to its children
  for (list<OperationPlan*>::const_iterator i = step_opplans.begin();
       i != step_opplans.end(); ++i)
  {
    (*i)->quantity = quantity;
    (*i)->resizeFlowLoadPlans();
  }
}


void OperationPlanRouting::eraseSubOperationPlan(OperationPlan* o)
{
  step_opplans.remove(o);
}


void OperationPlanRouting::setEnd(Date d)
{
  if(step_opplans.empty())
    OperationPlan::setEnd(d);
  else
  {
    // Move all sub-operationplans in an orderly fashion
    bool firstMove = true;
    for (list<OperationPlan*>::reverse_iterator i = step_opplans.rbegin();
         i != step_opplans.rend(); ++i)
    {
      if ((*i)->getDates().getEnd() > d || firstMove)
      {
        (*i)->setEnd(d);
        firstMove = false;
        d = (*i)->getDates().getStart();
      }
      else
        // There is sufficient slack in the routing
        break;
    }
    // Set the dates on the top operationplan
    setStartAndEnd(
      step_opplans.front()->getDates().getStart(),
      step_opplans.back()->getDates().getEnd()
    );
  }
}


void OperationPlanRouting::setStart(Date d)
{
  if(step_opplans.empty())
    OperationPlan::setStart(d);
  else
  {
    // Move all sub-operationplans in an orderly fashion
    bool firstMove = true;
    for (list<OperationPlan*>::const_iterator i = step_opplans.begin();
         i != step_opplans.end(); ++i)
    {
      if ((*i)->getDates().getStart() < d || firstMove)
      {
        (*i)->setStart(d);
        firstMove = false;
        d = (*i)->getDates().getEnd();
      }
      else
        // There is sufficient slack in the routing
        break;
    }
    // Set the dates on the top operationplan
    setStartAndEnd(
      step_opplans.front()->getDates().getStart(),
      step_opplans.back()->getDates().getEnd()
    );
  }
}


void OperationPlanRouting::update()
{
  if(!step_opplans.empty())
    // Set the dates on the top operationplan
    setStartAndEnd(
      step_opplans.front()->getDates().getStart(),
      step_opplans.back()->getDates().getEnd()
    );
  OperationPlan::update();
}


void OperationPlanRouting::initialize()
{
  // Create step suboperationplans if they don't exist yet.
  if (step_opplans.empty())
  {
    Date d = getDates().getEnd();
    OperationPlan *p = NULL;
    if (d)
    {
      // Using the end date
      for (Operation::Operationlist::const_reverse_iterator e =
        getOperation()->getSubOperations().rbegin();
        e != getOperation()->getSubOperations().rend(); ++e)
      {
        p = (*e)->createOperationPlan(getQuantity(), 0L,
          d, NULL, true, this, 0, true);
        d = p->getDates().getStart();
      }
    }
    else
    {
      // Using the start date when there is no end date
      d = getDates().getStart();
      // Using the current date when both the start and end date are missing
      if (!d) d = Plan::instance().getCurrent();
      for (Operation::Operationlist::const_iterator e =
        getOperation()->getSubOperations().begin();
        e != getOperation()->getSubOperations().end(); ++e)
      {
        p = (*e)->createOperationPlan(getQuantity(), d,
          0L, NULL, true, this, 0, true);
        d = p->getDates().getEnd();
      }
    }
  }

  // Initialize the suboperationplans
  for (list<OperationPlan*>::const_iterator i = step_opplans.begin();
       i != step_opplans.end(); ++i)
    (*i)->initialize();

  // Initialize myself
  OperationPlan::initialize();
}


void OperationPlanAlternate::addSubOperationPlan(OperationPlan* o)
{
  // The sub opplan must point to this operationplan
  assert(o->getOwner() == this);

  // Add in the list
  altopplan = o;

  // Update the top operationplan
  setStartAndEnd(
    altopplan->getDates().getStart(),
    altopplan->getDates().getEnd()
  );

  // Update if allowed
  if (runupdate) update();
}


OperationPlanAlternate::~OperationPlanAlternate()
{
  if (!altopplan) return;
  altopplan->owner = NULL;
  delete altopplan;
}


void OperationPlanAlternate::setEnd(Date d)
{
  if (!altopplan) return;
  altopplan->setEnd(d);
  setStartAndEnd(
    altopplan->getDates().getStart(),
    altopplan->getDates().getEnd()
  );
}


void OperationPlanAlternate::setStart(Date d)
{
  if (!altopplan) return;
  altopplan->setStart(d);
  setStartAndEnd(
    altopplan->getDates().getStart(),
    altopplan->getDates().getEnd()
  );
}


void OperationPlanAlternate::update()
{
  if (altopplan)
    setStartAndEnd(
      altopplan->getDates().getStart(),
      altopplan->getDates().getEnd()
    );
  OperationPlan::update();
}


void OperationPlanAlternate::initialize()
{
  // Create an alternate suboperationplan if one doesn't exist yet.
  // We use the first alternate by default.
  if (!altopplan && !getOperation()->getSubOperations().empty())
  {
    altopplan = getOperation()->getSubOperations().front()->
      createOperationPlan(getQuantity(), getDates().getStart(),
      getDates().getEnd(), NULL, true, this, 0, true);
  }

  // Initialize this operationplan and its child
  if (altopplan) altopplan->initialize();
  OperationPlan::initialize();
}


void OperationPlanAlternate::setQuantity(float f, bool roundDown)
{
  // First the normal resizing
  OperationPlan::setQuantity(f,roundDown);
  // Apply the same size also to the children operationplan
  if (altopplan)
  {
    altopplan->quantity = quantity;
    altopplan->resizeFlowLoadPlans();
  }
}


void OperationPlanAlternate::eraseSubOperationPlan(OperationPlan* o)
{
  if (altopplan == o)
    altopplan = NULL;
  else if (o)
    clog << "Warning: Trying to remove a sub operationplan '"
    << *(o->getOperation()) << "' that is not registered with"
    << " its parent '" << *getOperation() << "'" << endl;
}


void OperationPlanEffective::addSubOperationPlan(OperationPlan* o)
{
  // The sub opplan must point to this operationplan
  assert(o->getOwner() == this);

  // Add in the list
  effopplan = o;

  // Update the top operationplan
  setStartAndEnd(
    effopplan->getDates().getStart(),
    effopplan->getDates().getEnd()
  );

  // Update if allowed
  if (runupdate) update();
}


OperationPlanEffective::~OperationPlanEffective()
{
  if (!effopplan) return;
  effopplan->owner = NULL;
  delete effopplan;
}


void OperationPlanEffective::setEnd(Date d)
{
  if (!effopplan) return;
  effopplan->setEnd(d);
  setStartAndEnd(
    effopplan->getDates().getStart(),
    effopplan->getDates().getEnd()
  );
}


void OperationPlanEffective::setStart(Date d)
{
  if (!effopplan) return;
  effopplan->setStart(d);
  setStartAndEnd(
    effopplan->getDates().getStart(),
    effopplan->getDates().getEnd()
  );
}


void OperationPlanEffective::update()
{
  if (effopplan)
    setStartAndEnd(
      effopplan->getDates().getStart(),
      effopplan->getDates().getEnd()
    );
  OperationPlan::update();
}


void OperationPlanEffective::initialize()
{
  if (effopplan) effopplan->initialize();
  else throw LogicException("Can't initialize an effective operationplan " \
    "without suboperationplan");
  OperationPlan::initialize();
}


void OperationPlanEffective::setQuantity(float f, bool roundDown)
{
  // First the normal resizing
  OperationPlan::setQuantity(f,roundDown);
  // Apply the same size also to the children operationplan
  if (effopplan)
  {
    effopplan->quantity = quantity;
    effopplan->resizeFlowLoadPlans();
  }
}


void OperationPlanEffective::eraseSubOperationPlan(OperationPlan* o)
{
  if (effopplan == o)
    effopplan = NULL;
  else if (o)
    clog << "Warning: Trying to remove a sub operationplan '"
    << *(o->getOperation()) << "' that is not registered with"
    << " its parent '" << *getOperation() << "'" << endl;
}

}
