#!/usr/bin/python
#  file     : $URL$
#  revision : $LastChangedRevision$  $LastChangedBy$
#  date     : $LastChangedDate$
#  email    : jdetaeye@users.sourceforge.net

# This script is a simple, generic model generator. A number of different
# models are created with varying number of clusters, depth of the supply path
# and number of demands per cluster. By evaluating the runtime of these models
# we can evaluate different aspects of Frepple's scalability.
#
# This test script is meant more as a sample for your own tests on evaluating
# scalability.
#
# The autogenerated supply network looks schematically as follows:
#   [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
#   [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
#   [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
#   [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
#   [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
#       ...                                  ...
# Each row represents a cluster.
# The operation+buffer are repeated as many times as the depth of the supply
# path parameter.
# In each cluster a single item is defined, and a parametrizable number of
# demands is placed on the cluster.


from freppledb.input.models import *
import time, os, os.path, sys, random
from datetime import timedelta, date
from django.db import connection

# This function generates a random date
startdate = date(2007,1,1)
def getDate():
  global startdate
  return startdate + timedelta(random.uniform(0,365))

def erase_model():
  '''
  This routine erase all model data from the database.
  '''
  cursor = connection.cursor()
  cursor.execute('delete from frepple.output_problem')
  cursor.connection.commit()
  cursor.execute('delete from frepple.output_flowplan')
  cursor.connection.commit()
  cursor.execute('delete from frepple.output_loadplan')
  cursor.connection.commit()
  cursor.execute('delete from frepple.output_operationplan')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_demand')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_flow')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_load')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_buffer')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_resource')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_operationplan')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_item')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_operation')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_location')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_bucket')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_calendar')
  cursor.connection.commit()
  cursor.execute('delete from frepple.input_customer')
  cursor.connection.commit()

def create_model (cluster, demand, level):
  '''
  This routine populates the database with a sample dataset.
  '''
  # Initialization
  random.seed(100) # Initialize random seed to get reproducible results
  cnt = 100000     # a counter for operationplan identifiers

  # Create a random list of categories to choose from
  categories = [ 'cat A','cat B','cat C','cat D','cat E','cat F','cat G' ]

  # Create customers
  cust = []
  for i in range(100):
    c = Customer(name = 'Cust %03d' % i)
    cust.append(c)
    c.save()

  # Create resources and their calendars
  res = []
  for i in range(100):
    cal = Calendar(name='capacity for res %03d' %i, category='capacity')
    bkt = Bucket(start=date(2007,1,1), value=2, calendar=cal)
    cal.save()
    bkt.save()
    r = Resource(name = 'Res %03d' % i, maximum=cal)
    res.append(r)
    r.save()

  # Loop over all clusters
  for i in range(cluster):
    # location
    loc = Location(name='Loc %05d' % i)

    # Item and delivery operation
    oper = Operation(name='Del %05d' % i)
    it = Item(name='Itm %05d' % i, operation=oper, category=random.choice(categories))

    # Level 0 buffer
    buf = Buffer(name='Buf %05d L00' % i,
      item=it,
      location=loc,
      category='00'
      )
    fl = Flow(operation=oper, thebuffer=buf, quantity=-1)

    # Save the first batch
    loc.save()
    oper.save()
    it.save()
    fl.save()
    buf.save()

    # Demand
    for j in range(demand):
      dm = Demand(name='Dmd %05d %05d' % (i,j),
        item=it,
        quantity=int(random.uniform(1,11)),
        due=getDate(),
        priority=int(random.uniform(1,4)),
        customer=random.choice(cust),
        category=random.choice(categories)
        )
      dm.save()

    # Upstream operations and buffers
    for k in range(level):
      oper = Operation(name='Oper %05d L%02d' % (i,k))
      oper.save()
      if k == 1:
        # Create a resource load
        ld = Load(resource=random.choice(res), operation=oper)
        ld.save()
      buf.producing = oper
      fl = Flow(operation=oper, thebuffer=buf, quantity=1, type="FLOW_END")
      buf.save()
      fl.save()
      buf = Buffer(name='Buf %05d L%02d' % (i,k+1),
        item=it,
        location=loc,
        category='%02d' % (k+1)
        )
      fl = Flow(operation=oper, thebuffer=buf, quantity=-1)
      buf.save()
      fl.save()

    # Create supply operation
    oper = Operation(name='Sup %05d' % i)
    fl = Flow(operation=oper, thebuffer=buf, quantity=1)
    oper.save()
    fl.save()

    # Create actual supply
    for i in range(demand/10):
        cnt += 1
        opplan = OperationPlan(identifier=cnt, operation=oper, quantity=int(random.uniform(1,100)), start=getDate(), end=getDate())
        opplan.save()
