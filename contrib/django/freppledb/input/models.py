#
# Copyright (C) 2007 by Johan De Taeye
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
#

# file : $URL$
# revision : $LastChangedRevision$  $LastChangedBy$
# date : $LastChangedDate$
# email : jdetaeye@users.sourceforge.net

from django.db import models
from datetime import date
import sys
from django.http import HttpRequest
from datetime import datetime

# The date format used by the frepple XML data.
dateformat = '%Y-%m-%dT%H:%M:%S'

class Plan(models.Model):
    name = models.CharField(maxlength=60)
    description = models.CharField(maxlength=60, blank=True)
    current = models.DateTimeField('current date')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    class Admin:
        list_display = ('name', 'description', 'current')
        search_fields = ['name', 'description']
    class Meta:
        verbose_name_plural = 'Plan'  # There will only be 1 plan...

class Calendar(models.Model):
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def currentvalue(self):
        ''' Returns the value of the calendar on the current day.'''
        v = 0.0
        cur = date.today()
        for b in self.buckets.all():
            if cur < b.start: return v
            v = b.value
        return v
    currentvalue.short_description = 'Current value'
    def __str__(self):
        return self.name
    class Admin:
        list_display = ('name', 'description', 'category', 'subcategory', 'currentvalue', 'lastmodified')
        search_fields = ['name','description']
        list_filter = ['category', 'subcategory']
        save_as = True

class Bucket(models.Model):
    calendar = models.ForeignKey(Calendar, edit_inline=models.TABULAR, min_num_in_admin=5, num_extra_on_change=3, related_name='buckets')
    start = models.DateField('start date', core=True)
    name = models.CharField(maxlength=60, null=True, blank=True)
    value = models.FloatField(max_digits=10, decimal_places=2, default=0.00)
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        if self.name: return self.name
        return str(self.start)
    class Meta:
        ordering = ('start','name')

class Location(models.Model):
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    owner = models.ForeignKey('self', null=True, blank=True, related_name='children',
      raw_id_admin=True, help_text='Hierarchical parent')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    class Admin:
        list_display = ('name', 'description', 'category', 'subcategory', 'owner', 'lastmodified')
        search_fields = ['name', 'description']
        list_filter = ['category', 'subcategory']
        save_as = True

class Customer(models.Model):
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    owner = models.ForeignKey('self', null=True, blank=True, related_name='children',
      raw_id_admin=True, help_text='Hierarchical parent')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    class Admin:
        list_display = ('name', 'description', 'category', 'subcategory', 'owner', 'lastmodified')
        search_fields = ['name', 'description']
        list_filter = ['category', 'subcategory']
        save_as = True

class Item(models.Model):
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    operation = models.ForeignKey('Operation', null=True, blank=True, raw_id_admin=True)
    owner = models.ForeignKey('self', null=True, blank=True, related_name='children',
      raw_id_admin=True, help_text='Hierarchical parent')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    class Admin:
        list_display = ('name', 'description', 'category', 'subcategory', 'operation', 'owner', 'lastmodified')
        search_fields = ['name', 'description']
        list_filter = ['category', 'subcategory']
        save_as = True

class Operation(models.Model):
    operationtypes = (
      ('','FIXED_TIME'),
      ('OPERATION_FIXED_TIME','FIXED_TIME'),
      ('OPERATION_TIME_PER','TIME_PER'),
      ('OPERATION_ROUTING','ROUTING'),
      ('OPERATION_ALTERNATE','ALTERNATE'),
    )
    name = models.CharField(maxlength=60, primary_key=True)
    type = models.CharField(maxlength=20, null=True, blank=True, choices=operationtypes, default='')
    fence = models.FloatField('release fence', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="Operationplans within this time window from the current day are expected to be released to production ERP")
    pretime = models.FloatField('pre-op time', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="A delay time to be respected as a soft constraint before starting the operation")
    posttime = models.FloatField('post-op time', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="A delay time to be respected as a soft constraint after ending the operation")
    sizeminimum = models.FloatField('size minimum', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="A minimum lotsize quantity for operationplans")
    sizemultiple = models.FloatField('size multiple', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="A multiple quantity for operationplans")
    duration = models.FloatField('duration', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="A fixed duration for the operation")
    duration_per = models.FloatField('duration per unit', max_digits=10, decimal_places=2, null=True, blank=True,
      help_text="A variable duration for the operation")
    # Putting the priority and owner fields in this table means that an operation can have only 1 parent.
    # Internally however, Frepple can re-use the same operation in multiple hierarchies, ie an operation
    # can have multiple parents.
    priority = models.PositiveIntegerField(null=True, blank=True,
      help_text="Priority of this operation among the members of the owning operation")
    owner = models.ForeignKey('self', null=True, blank=True, related_name='suboperations', # todo NOT good, owner field is not read!!!
      raw_id_admin=True, help_text='Hierarchical parent')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    def save(self):
        if self.type == '' or self.type == 'OPERATION_FIXED_TIME':
          self.duration_per = None
        elif self.type != 'OPERATION_TIME_PER':
          self.duration = None
          self.duration_per = None
        # Call the real save() method
        super(Operation, self).save()
    class Admin:
        list_display = ('name', 'type', 'fence', 'pretime', 'posttime', 'sizeminimum', 'sizemultiple', 'owner', 'priority', 'lastmodified')
        search_fields = ['name',]
        save_as = True
        fields = (
            (None, {'fields': ('name', 'type')}),
            ('Hierarchy', {'fields': ('owner', 'priority')}),
            ('Planning parameters', {'fields': ('fence', 'pretime', 'posttime', 'sizeminimum', 'sizemultiple', 'duration', 'duration_per')}),
        )

class Buffer(models.Model):
    buffertypes = (
      ('','Default'),
      ('BUFFER_INFINITE','Infinite'),
    )
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    type = models.CharField(maxlength=20, null=True, blank=True, choices=buffertypes, default='')
    location = models.ForeignKey(Location, null=True, blank=True, db_index=True, raw_id_admin=True)
    item = models.ForeignKey(Item, db_index=True, raw_id_admin=True)
    onhand = models.FloatField(max_digits=10, decimal_places=2, default=0.00, help_text='current inventory')
    minimum = models.ForeignKey('Calendar', null=True, blank=True, raw_id_admin=True,
      help_text='Calendar storing the safety stock profile')
    producing = models.ForeignKey('Operation', null=True, blank=True,
      related_name='used_producing', raw_id_admin=True,
      help_text='Operation to replenish the buffer')
    consuming = models.ForeignKey('Operation', null=True, blank=True,
      related_name='used_consuming', raw_id_admin=True,
      help_text='Operation to move extra inventory from the buffer')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    def save(self):
        if self.type == 'BUFFER_INFINITE':
            # These fields are not relevant for infinite buffers
            self.minimum = None
            self.producing = None
            self.consuming = None
        # Call the real save() method
        super(Buffer, self).save()
    class Admin:
        fields = (
            (None,{
              'fields': (('name', 'item', 'location'), 'description', ('category', 'subcategory'))}),
            ('Inventory', {
              'fields': ('onhand',)}),
            ('Planning parameters', {
              'fields': ('type','minimum','producing','consuming'),
              'classes': 'collapse'},),
        )
        # @todo hide irrelevant fields: feature not available in official releases yet
        #hide_unless = {
        #  'minimum': {'type': ''}
        #  'producing': {'type': ''}
        #  'consuming': {'type': ''}
        #}
        list_display = ('name', 'description', 'category', 'subcategory', 'location', 'item',
          'onhand', 'type', 'minimum', 'producing', 'consuming', 'lastmodified')
        search_fields = ['name', 'description']
        list_filter = ['category', 'subcategory']
        save_as = True

class Resource(models.Model):
    resourcetypes = (
      ('','Default'),
      ('RESOURCE_INFINITE','Infinite'),
    )
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    type = models.CharField(maxlength=20, null=True, blank=True, choices=resourcetypes, default='')
    maximum = models.ForeignKey('Calendar', null=True, blank=True,
      raw_id_admin=True, help_text='Calendar defining the available capacity')
    location = models.ForeignKey(Location, null=True, blank=True, db_index=True, raw_id_admin=True)
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    def save(self):
        if self.type == 'RESOURCE_INFINITE':
            # These fields are not relevant for infinite resources
            self.maximum = None
        # Call the real save() method
        super(Resource, self).save()
    class Admin:
        list_display = ('name', 'description', 'category', 'subcategory', 'location',
          'type', 'maximum', 'lastmodified')
        search_fields = ['name', 'description']
        list_filter = ['category', 'subcategory']
        save_as = True

class Flow(models.Model):
    flowtypes = (
      ('','Start'),
      ('FLOW_END','End'),
    )
    operation = models.ForeignKey(Operation, db_index=True, raw_id_admin=True)
    thebuffer = models.ForeignKey(Buffer, db_index=True, raw_id_admin=True)
    type = models.CharField(maxlength=20, null=True, blank=True,
      choices=flowtypes,
      default='',
      help_text='Consume/produce material at the start or the end of the operationplan',
      )
    quantity = models.FloatField(max_digits=10, decimal_places=2, default='1.00')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return '%s - %s' % (self.operation.name, self.thebuffer.name)
    class Admin:
        save_as = True
        search_fields = ['operation', 'thebuffer']
        list_display = ('operation', 'thebuffer', 'type', 'quantity', 'lastmodified')
        # @todo we don't have a hyperlink any more to edit a flow...
        list_display_links = ('operation', 'thebuffer')
    class Meta:
        unique_together = (('operation','thebuffer'),)

class Load(models.Model):
    operation = models.ForeignKey(Operation, db_index=True, raw_id_admin=True)
    resource = models.ForeignKey(Resource, db_index=True, raw_id_admin=True)
    usagefactor = models.FloatField(max_digits=10, decimal_places=2, default='1.00')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return '%s - %s' % (self.operation.name, self.resource.name)
    class Admin:
        search_fields = ['operation', 'resource']
        list_display = ('operation', 'resource', 'usagefactor', 'lastmodified')
        save_as = True
    class Meta:
        unique_together = (('operation','resource'),)

class OperationPlan(models.Model):
    identifier = models.IntegerField(primary_key=True,
      help_text='Unique identifier of an operationplan')
    operation = models.ForeignKey(Operation, db_index=True, raw_id_admin=True)
    quantity = models.FloatField(max_digits=10, decimal_places=2, default='1.00')
    start = models.DateTimeField(help_text='Start date')
    end = models.DateTimeField(help_text='End date')
    locked = models.BooleanField(default=True, radio_admin=True, help_text='Prevent or allow changes')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return str(self.identifier)
    class Admin:
        save_as = True
        search_fields = ['operation']
        list_display = ('identifier', 'operation', 'start', 'end', 'quantity', 'locked', 'lastmodified')
        date_hierarchy = 'start'

class Demand(models.Model):
    # The priorities defined here are for convenience only. Frepple accepts any number as priority.
    demandpriorities = (
      (1,'1 - high'),
      (2,'2 - normal'),
      (3,'3 - low')
    )
    demandpolicies = (
      ('','late with multiple deliveries'),
      ('SINGLEDELIVERY','late with single delivery'),
      ('PLANSHORT', 'short with multiple deliveries'),
      ('PLANSHORT SINGLEDELIVERY', 'short with single delivery')
    )
    name = models.CharField(maxlength=60, primary_key=True)
    description = models.CharField(maxlength=200, null=True, blank=True)
    category = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    subcategory = models.CharField(maxlength=20, null=True, blank=True, db_index=True)
    customer = models.ForeignKey(Customer, null=True, blank=True, db_index=True, raw_id_admin=True)
    item = models.ForeignKey(Item, db_index=True, raw_id_admin=True)
    due = models.DateField('due')
    operation = models.ForeignKey('Operation', null=True, blank=True,
      related_name='used_demand', raw_id_admin=True, help_text='Operation used to satisfy this demand')
    quantity = models.FloatField(max_digits=10, decimal_places=2)
    priority = models.PositiveIntegerField(default=2, choices=demandpriorities, radio_admin=True)
    policy = models.CharField(maxlength=25, null=True, blank=True, choices=demandpolicies,
      help_text='Choose whether to plan the demand short or late, and with single or multiple deliveries allowed')
    owner = models.ForeignKey('self', null=True, blank=True, raw_id_admin=True,
      help_text='Hierarchical parent')
    lastmodified = models.DateTimeField('last modified', auto_now=True, editable=False, db_index=True)
    def __str__(self):
        return self.name
    class Admin:
        fields = (
            (None, {'fields': ('name', 'item', 'customer', 'description', 'category','subcategory', 'due', 'quantity', 'priority','owner')}),
            ('Planning parameters', {'fields': ('operation', 'policy', ), 'classes': 'collapse'}),
        )
        list_display = ('name', 'item', 'customer', 'description', 'category',
          'subcategory', 'due', 'operation', 'quantity', 'priority','owner', 'lastmodified')
        search_fields = ['name','customer','item','operation']
        date_hierarchy = 'due'
        list_filter = ['due','priority','category','subcategory']
        save_as = True
