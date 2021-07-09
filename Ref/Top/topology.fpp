module Ref {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers 
  # ----------------------------------------------------------------------

  module Ports {

    enum CmdDispatcher {
      sequencer
      uplink
    }

    enum RateGroups {
      rateGroup1
      rateGroup2
      rateGroup3
    }

    enum StaticMemory {
      downlink
      uplink
    }

  }

  topology Ref {

    # ----------------------------------------------------------------------
    # Instances used in the topology
    # ----------------------------------------------------------------------

    instance $health
    instance SG1
    instance SG2
    instance SG3
    instance SG4
    instance SG5
    instance blockDrv
    instance chanTlm
    instance cmdDisp
    instance cmdSeq
    instance comm
    instance downlink
    instance eventLogger
    instance fatalAdapter
    instance fatalHandler
    instance fileDownlink
    instance fileManager
    instance fileUplink
    instance fileUplinkBufferManager
    instance linuxTime
    instance pingRcvr
    instance prmDb
    instance rateGroup1Comp
    instance rateGroup2Comp
    instance rateGroup3Comp
    instance rateGroupDriverComp
    instance recvBuffComp
    instance sendBuffComp
    instance staticMemory
    instance textLogger
    instance uplink

    # ----------------------------------------------------------------------
    # Pattern graph specifiers
    # ----------------------------------------------------------------------

    command connections instance cmdDisp

    event connections instance eventLogger

    param connections instance prmDb

    telemetry connections instance chanTlm

    text event connections instance textLogger

    time connections instance linuxTime

    health connections instance $health

    # ----------------------------------------------------------------------
    # Direct graph specifiers
    # ----------------------------------------------------------------------

    connections Downlink {

      chanTlm.PktSend -> downlink.comIn
      eventLogger.PktSend -> downlink.comIn
      fileDownlink.bufferSendOut -> downlink.bufferIn

      downlink.framedAllocate -> staticMemory.bufferAllocate[Ports.StaticMemory.downlink]
      downlink.framedOut -> comm.send
      downlink.bufferDeallocate -> fileDownlink.bufferReturn

      comm.deallocate -> staticMemory.bufferDeallocate[Ports.StaticMemory.downlink]

    }

    connections RateGroups {

      # Block driver
      blockDrv.CycleOut -> rateGroupDriverComp.CycleIn

      # Rate group 1
      rateGroupDriverComp.CycleOut[Ports.RateGroups.rateGroup1] -> rateGroup1Comp.CycleIn
      rateGroup1Comp.RateGroupMemberOut[0] -> SG1.schedIn
      rateGroup1Comp.RateGroupMemberOut[1] -> SG2.schedIn
      rateGroup1Comp.RateGroupMemberOut[2] -> chanTlm.Run
      rateGroup1Comp.RateGroupMemberOut[3] -> fileDownlink.Run

      # Rate group 2
      rateGroupDriverComp.CycleOut[Ports.RateGroups.rateGroup2] -> rateGroup2Comp.CycleIn
      rateGroup2Comp.RateGroupMemberOut[0] -> cmdSeq.schedIn
      rateGroup2Comp.RateGroupMemberOut[1] -> sendBuffComp.SchedIn
      rateGroup2Comp.RateGroupMemberOut[2] -> SG3.schedIn
      rateGroup2Comp.RateGroupMemberOut[3] -> SG4.schedIn

      # Rate group 3
      rateGroupDriverComp.CycleOut[Ports.RateGroups.rateGroup3] -> rateGroup3Comp.CycleIn
      rateGroup3Comp.RateGroupMemberOut[0] -> $health.Run
      rateGroup3Comp.RateGroupMemberOut[1] -> SG5.schedIn
      rateGroup3Comp.RateGroupMemberOut[2] -> blockDrv.Sched
      rateGroup3Comp.RateGroupMemberOut[3] -> fileUplinkBufferManager.schedIn

    }

    connections FaultProtection {
      eventLogger.FatalAnnounce -> fatalHandler.FatalReceive
    }

    connections Ref {
      sendBuffComp.Data -> blockDrv.BufferIn
      blockDrv.BufferOut -> recvBuffComp.Data
    }

    connections Sequencer {
      cmdDisp.seqCmdStatus[Ports.CmdDispatcher.sequencer] -> cmdSeq.cmdResponseIn
      cmdSeq.comCmdOut -> cmdDisp.seqCmdBuff[Ports.CmdDispatcher.sequencer]
    }

    connections Uplink {

      comm.allocate -> staticMemory.bufferAllocate[Ports.StaticMemory.uplink]
      comm.$recv -> uplink.framedIn

      uplink.bufferAllocate -> fileUplinkBufferManager.bufferGetCallee
      uplink.comOut -> cmdDisp.seqCmdBuff[Ports.CmdDispatcher.uplink]
      uplink.bufferOut -> fileUplink.bufferSendIn
      uplink.framedDeallocate -> staticMemory.bufferDeallocate[Ports.StaticMemory.uplink]
      uplink.bufferDeallocate -> fileUplinkBufferManager.bufferSendIn

      fileUplink.bufferSendOut -> fileUplinkBufferManager.bufferSendIn

    }

  }

}
