# Basic Transform Replicator
Basic Transform Replicator sends your actors transform to every client and smooths the location difference between server updates.
Also tries its best to replicate physics but please note UE4 is not really capable of physics replication.

Currently built for 4.26

Can not be used as movement component. Does not quite work well for AutonomousProxies, since there isn't prediction and reconciliation is implemented.

You can check my plugin on marketplace if you need an optimized networked component that works for AutonomousProxies: https://www.unrealengine.com/marketplace/en-US/product/replicated-pawn-movement-component-intax-movement

I made this plugin on my free time, took me a hour or less. So just posting there if someone needs a simple solution for its simple replication problems :)

Cheers, Intax.
