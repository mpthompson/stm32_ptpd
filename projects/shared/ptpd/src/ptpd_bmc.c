#include <string.h>
#include "ptpd.h"

#if LWIP_PTPD

// Convert EUI48 format to EUI64.
static void eui48_to_eui64(const octet_t * eui48, octet_t * eui64)
{
  eui64[0] = eui48[0];
  eui64[1] = eui48[1];
  eui64[2] = eui48[2];
  eui64[3] = 0xff;
  eui64[4] = 0xfe;
  eui64[5] = eui48[3];
  eui64[6] = eui48[4];
  eui64[7] = eui48[5];
}

// Initialize the ptp_clock with run time values.
void ptpd_clock_init(PtpClock *ptp_clock)
{
  RunTimeOpts *rtOpts;

  DBG("PTPD: ptpd_clock_init\n");

  // Point to the runtime options.
  rtOpts = &ptp_clock->rtOpts;

  // Default data set.
  ptp_clock->defaultDS.twoStepFlag = DEFAULT_TWO_STEP_FLAG;

  // Init clockIdentity with MAC address and 0xFF and 0xFE. see spec 7.5.2.2.2.
  if ((CLOCK_IDENTITY_LENGTH == 8) && (PTP_UUID_LENGTH == 6))
  {
    DBGVV("ptpd_clock_init: eui48_to_eui64\n");
    eui48_to_eui64(ptp_clock->portUuidField, ptp_clock->defaultDS.clockIdentity);
  }
  else if (CLOCK_IDENTITY_LENGTH == PTP_UUID_LENGTH)
  {
    memcpy(ptp_clock->defaultDS.clockIdentity, ptp_clock->portUuidField, CLOCK_IDENTITY_LENGTH);
  }
  else
  {
    ERROR("ptpd_clock_init: UUID length is not valid");
  }

  ptp_clock->defaultDS.numberPorts = NUMBER_PORTS;

  ptp_clock->defaultDS.clockQuality.clockAccuracy = rtOpts->clockQuality.clockAccuracy;
  ptp_clock->defaultDS.clockQuality.clockClass = rtOpts->clockQuality.clockClass;
  ptp_clock->defaultDS.clockQuality.offsetScaledLogVariance = rtOpts->clockQuality.offsetScaledLogVariance;

  ptp_clock->defaultDS.priority1 = rtOpts->priority1;
  ptp_clock->defaultDS.priority2 = rtOpts->priority2;

  ptp_clock->defaultDS.domainNumber = rtOpts->domainNumber;
  ptp_clock->defaultDS.slaveOnly = rtOpts->slaveOnly;

  // Port configuration data set.

  // PortIdentity Init (portNumber = 1 for an ardinary clock spec 7.5.2.3).
  memcpy(ptp_clock->portDS.portIdentity.clockIdentity, 
         ptp_clock->defaultDS.clockIdentity,
         CLOCK_IDENTITY_LENGTH);
  ptp_clock->portDS.portIdentity.portNumber = NUMBER_PORTS;
  ptp_clock->portDS.logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;
  ptp_clock->portDS.peerMeanPathDelay.seconds = ptp_clock->portDS.peerMeanPathDelay.nanoseconds = 0;
  ptp_clock->portDS.logAnnounceInterval = rtOpts->announceInterval;
  ptp_clock->portDS.announceReceiptTimeout = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
  ptp_clock->portDS.logSyncInterval = rtOpts->syncInterval;
  ptp_clock->portDS.delayMechanism = rtOpts->delayMechanism;
  ptp_clock->portDS.logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
  ptp_clock->portDS.versionNumber = VERSION_PTP;

  // Initialize other stuff.
  ptp_clock->foreignMasterDS.count = 0;
  ptp_clock->foreignMasterDS.capacity = rtOpts->maxForeignRecords;

  ptp_clock->inboundLatency = rtOpts->inboundLatency;
  ptp_clock->outboundLatency = rtOpts->outboundLatency;

  ptp_clock->servo.sDelay = rtOpts->servo.sDelay;
  ptp_clock->servo.sOffset = rtOpts->servo.sOffset;
  ptp_clock->servo.ai = rtOpts->servo.ai;
  ptp_clock->servo.ap = rtOpts->servo.ap;
  ptp_clock->servo.noAdjust = rtOpts->servo.noAdjust;
  ptp_clock->servo.noResetClock = rtOpts->servo.noResetClock;

  ptp_clock->stats = rtOpts->stats;
}

// Compare two port identities.
bool ptpd_is_same_port_identity(const PortIdentity *a, const PortIdentity *b)
{
  // Compare the clock identities and the port numbers.
  return (bool)((memcmp(a->clockIdentity, b->clockIdentity, CLOCK_IDENTITY_LENGTH) == 0) && 
                (a->portNumber == b->portNumber));
}

// Add foreign record defined by announce message.
void ptpd_add_foreign(PtpClock *ptp_clock, const MsgHeader *header, const MsgAnnounce *announce)
{
  int i, j;
  bool found = false;

  j = ptp_clock->foreignMasterDS.best;

  // Check if Foreign master is already known.
  for (i = 0; i < ptp_clock->foreignMasterDS.count; i++)
  {
    if (ptpd_is_same_port_identity(&header->sourcePortIdentity, &ptp_clock->foreignMasterDS.records[j].foreignMasterPortIdentity))
    {
      // Foreign Master is already in Foreignmaster data set.
      found = true;
      ptp_clock->foreignMasterDS.records[j].foreignMasterAnnounceMessages++;
      DBGV("PTPD: ptpd_add_foreign: AnnounceMessage incremented\n");
      ptp_clock->foreignMasterDS.records[j].header = *header;
      ptp_clock->foreignMasterDS.records[j].announce = *announce;
      break;
    }

    j = (j + 1) % ptp_clock->foreignMasterDS.count;
  }

  // If not found, we have a new foreign master.
  if (!found)
  {
    if (ptp_clock->foreignMasterDS.count < ptp_clock->foreignMasterDS.capacity)
    {
      ptp_clock->foreignMasterDS.count++;
    }

    j = ptp_clock->foreignMasterDS.i;

    // Copy new foreign master data set from Announce message.
    memcpy(ptp_clock->foreignMasterDS.records[j].foreignMasterPortIdentity.clockIdentity, 
           header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
    ptp_clock->foreignMasterDS.records[j].foreignMasterPortIdentity.portNumber = header->sourcePortIdentity.portNumber;
    ptp_clock->foreignMasterDS.records[j].foreignMasterAnnounceMessages = 0;

    // Header and announce field of each foreign master are useful to run Best Master Clock Algorithm.
    ptp_clock->foreignMasterDS.records[j].header = *header;
    ptp_clock->foreignMasterDS.records[j].announce = *announce;

    DBGV("PTPD: ptpd_add_foreign: New foreign master added\n");

    ptp_clock->foreignMasterDS.i = (ptp_clock->foreignMasterDS.i + 1) % ptp_clock->foreignMasterDS.capacity;
  }
}

// Local clock is becoming Master. Table 13 (9.3.5) of the spec.
// When recommended state is Master, copy local data into parent and grandmaster dataset.
void ptpd_m1(PtpClock *ptp_clock)
{
  DBGV("PTPD: ptpd_m1\n");

  // Current data set update.
  ptp_clock->currentDS.stepsRemoved = 0;
  ptp_clock->currentDS.offsetFromMaster.seconds = ptp_clock->currentDS.offsetFromMaster.nanoseconds = 0;
  ptp_clock->currentDS.meanPathDelay.seconds = ptp_clock->currentDS.meanPathDelay.nanoseconds = 0;

  // Parent data set.
  memcpy(ptp_clock->parentDS.parentPortIdentity.clockIdentity, ptp_clock->defaultDS.clockIdentity, CLOCK_IDENTITY_LENGTH);
  ptp_clock->parentDS.parentPortIdentity.portNumber = 0;
  memcpy(ptp_clock->parentDS.grandmasterIdentity, ptp_clock->defaultDS.clockIdentity, CLOCK_IDENTITY_LENGTH);
  ptp_clock->parentDS.grandmasterClockQuality.clockAccuracy = ptp_clock->defaultDS.clockQuality.clockAccuracy;
  ptp_clock->parentDS.grandmasterClockQuality.clockClass = ptp_clock->defaultDS.clockQuality.clockClass;
  ptp_clock->parentDS.grandmasterClockQuality.offsetScaledLogVariance = ptp_clock->defaultDS.clockQuality.offsetScaledLogVariance;
  ptp_clock->parentDS.grandmasterPriority1 = ptp_clock->defaultDS.priority1;
  ptp_clock->parentDS.grandmasterPriority2 = ptp_clock->defaultDS.priority2;

  // Time properties data set.
  ptp_clock->timePropertiesDS.currentUtcOffset = ptp_clock->rtOpts.currentUtcOffset;
  ptp_clock->timePropertiesDS.currentUtcOffsetValid = DEFAULT_UTC_VALID;
  ptp_clock->timePropertiesDS.leap59 = false;
  ptp_clock->timePropertiesDS.leap61 = false;
  ptp_clock->timePropertiesDS.timeTraceable = DEFAULT_TIME_TRACEABLE;
  ptp_clock->timePropertiesDS.frequencyTraceable = DEFAULT_FREQUENCY_TRACEABLE;
  ptp_clock->timePropertiesDS.ptpTimescale = (bool)(DEFAULT_TIMESCALE == PTP_TIMESCALE);
  ptp_clock->timePropertiesDS.timeSource = DEFAULT_TIME_SOURCE;
}

// Local clock is becoming Master. Table 13 (9.3.5) of the spec.
void ptpd_m2(PtpClock *ptp_clock)
{
  DBGV("PTPD: ptpd_m2\n");

  // For now, ptpd_m1 and ptpd_m2 are equivalent.
  ptpd_m1(ptp_clock);
}

// When recommended state is Passive.
void ptpd_p1(PtpClock *ptp_clock)
{
  DBGV("PTPD: ptpd_p1\n");
}

// Local clock is synchronized to Ebest Table 16 (9.3.5) of the spec.
// When recommended state is Slave, copy dataset of master into parent and grandmaster dataset.
void ptpd_s1(PtpClock *ptp_clock, const MsgHeader *header, const MsgAnnounce *announce)
{
  DBGV("PTPD: ptpd_s1\n");

  // Current DS.
  ptp_clock->currentDS.stepsRemoved = announce->stepsRemoved + 1;

  if (!ptpd_is_same_port_identity(&ptp_clock->parentDS.parentPortIdentity, &header->sourcePortIdentity))
  {
      set_flag(ptp_clock->events, MASTER_CLOCK_CHANGED);
  }

  // Parent DS.
  memcpy(ptp_clock->parentDS.parentPortIdentity.clockIdentity,
         header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
  ptp_clock->parentDS.parentPortIdentity.portNumber = header->sourcePortIdentity.portNumber;
  memcpy(ptp_clock->parentDS.grandmasterIdentity,
         announce->grandmasterIdentity, CLOCK_IDENTITY_LENGTH);
  ptp_clock->parentDS.grandmasterClockQuality.clockAccuracy = announce->grandmasterClockQuality.clockAccuracy;
  ptp_clock->parentDS.grandmasterClockQuality.clockClass = announce->grandmasterClockQuality.clockClass;
  ptp_clock->parentDS.grandmasterClockQuality.offsetScaledLogVariance = announce->grandmasterClockQuality.offsetScaledLogVariance;
  ptp_clock->parentDS.grandmasterPriority1 = announce->grandmasterPriority1;
  ptp_clock->parentDS.grandmasterPriority2 = announce->grandmasterPriority2;

  // Time properties DS.
  ptp_clock->timePropertiesDS.currentUtcOffset = announce->currentUtcOffset;
  ptp_clock->timePropertiesDS.currentUtcOffsetValid = get_flag(header->flagField[1], FLAG1_UTC_OFFSET_VALID);
  ptp_clock->timePropertiesDS.leap59 = get_flag(header->flagField[1], FLAG1_LEAP59);
  ptp_clock->timePropertiesDS.leap61 = get_flag(header->flagField[1], FLAG1_LEAP61);
  ptp_clock->timePropertiesDS.timeTraceable = get_flag(header->flagField[1], FLAG1_TIME_TRACEABLE);
  ptp_clock->timePropertiesDS.frequencyTraceable = get_flag(header->flagField[1], FLAG1_FREQUENCY_TRACEABLE);
  ptp_clock->timePropertiesDS.ptpTimescale = get_flag(header->flagField[1], FLAG1_PTP_TIMESCALE);
  ptp_clock->timePropertiesDS.timeSource = announce->timeSource;
}

#define A_better_then_B 1
#define B_better_then_A -1
#define A_better_by_topology_then_B 1
#define B_better_by_topology_then_A -1
#define ERROR_1 0
#define ERROR_2 -0

#define COMPARE_AB_RETURN_BETTER(cond, msg)                             \
  if ((announce_a->cond) > (announce_b->cond)) {                        \
    DBGVV("PTPD: ptpd_data_set_comparison: " msg ": B better then A\n");      \
    return B_better_then_A;                                             \
  }                                                                     \
  if ((announce_b->cond) > (announce_a->cond)) {                        \
    DBGVV("PTPD: ptpd_data_set_comparison: " msg ": A better then B\n");      \
    return A_better_then_B;                                             \
  }                                                                     \

// Copy local data set into header and announce message. 9.3.4 table 12.
static void ptpd_copy_d0(MsgHeader *header, MsgAnnounce *announce, PtpClock *ptp_clock)
{
  announce->grandmasterPriority1 = ptp_clock->defaultDS.priority1;
  memcpy(announce->grandmasterIdentity, ptp_clock->defaultDS.clockIdentity, CLOCK_IDENTITY_LENGTH);
  announce->grandmasterClockQuality.clockClass = ptp_clock->defaultDS.clockQuality.clockClass;
  announce->grandmasterClockQuality.clockAccuracy = ptp_clock->defaultDS.clockQuality.clockAccuracy;
  announce->grandmasterClockQuality.offsetScaledLogVariance = ptp_clock->defaultDS.clockQuality.offsetScaledLogVariance;
  announce->grandmasterPriority2 = ptp_clock->defaultDS.priority2;
  announce->stepsRemoved = 0;
  memcpy(header->sourcePortIdentity.clockIdentity, ptp_clock->defaultDS.clockIdentity, CLOCK_IDENTITY_LENGTH);
}

// Data set comparison bewteen two foreign masters (9.3.4 fig 27). Return similar to memcmp().
static int8_t ptpd_data_set_comparison(MsgHeader *header_a, MsgAnnounce *announce_a,
                                       MsgHeader *header_b, MsgAnnounce *announce_b,
                                       PtpClock *ptp_clock)
{
  int grandmaster_identity_comp;
  short comp = 0;

  DBGV("PTPD: ptpd_data_set_comparison\n");

  // GM identity of A == GM identity of B 
  grandmaster_identity_comp = memcmp(announce_a->grandmasterIdentity, announce_b->grandmasterIdentity, CLOCK_IDENTITY_LENGTH);

  // Algoritgm part 1 - Figure 27.
  if (grandmaster_identity_comp != 0)
  {
    COMPARE_AB_RETURN_BETTER(grandmasterPriority1, "grandmaster.Priority1");
    COMPARE_AB_RETURN_BETTER(grandmasterClockQuality.clockClass, "grandmaster.clockClass");
    COMPARE_AB_RETURN_BETTER(grandmasterClockQuality.clockAccuracy, "grandmaster.clockAccuracy");
    COMPARE_AB_RETURN_BETTER(grandmasterClockQuality.offsetScaledLogVariance, "grandmaster.Variance");
    COMPARE_AB_RETURN_BETTER(grandmasterPriority2, "grandmaster.Priority2");

    if (grandmaster_identity_comp > 0)
    {
      DBGVV("PTPD: ptpd_data_set_comparison: grandmaster.Identity: B better then A\n");
      return B_better_then_A;
    }
    else if (grandmaster_identity_comp < 0)
    {
      DBGVV("PTPD: ptpd_data_set_comparison: grandmaster.Identity: A better then B\n");
      return A_better_then_B;
    }
  }

  // Algoritgm part 2 - Figure 28.
  if ((announce_a->stepsRemoved) > (announce_b->stepsRemoved + 1))
  {
    DBGVV("PTPD: ptpd_data_set_comparison: stepsRemoved: B better then A\n");
    return B_better_then_A;
  }
  if ((announce_b->stepsRemoved) > (announce_a->stepsRemoved + 1))
  {
    DBGVV("PTPD: ptpd_data_set_comparison: stepsRemoved: A better then B\n");
    return A_better_then_B;
  }
  if ((announce_a->stepsRemoved) > (announce_b->stepsRemoved))
  {
    comp = memcmp(header_a->sourcePortIdentity.clockIdentity,
                  ptp_clock->portDS.portIdentity.clockIdentity,
                  CLOCK_IDENTITY_LENGTH);

    if (comp > 0)
    {
      // Reciever less than sender.
      DBGVV("PTPD: ptpd_data_set_comparison: PortIdentity: B better then A\n");
      return B_better_then_A;
    }
    else if (comp < 0)
    {
      // Receiver greater than sender.
      DBGVV("PTPD: ptpd_data_set_comparison: PortIdentity: B better by topology then A\n");
      return B_better_by_topology_then_A;
    }

    DBGVV("PTPD: ptpd_data_set_comparison: ERROR 1\n");
    return ERROR_1;
  }
  if ((announce_a->stepsRemoved) < (announce_b->stepsRemoved))
  {
    comp = memcmp(header_b->sourcePortIdentity.clockIdentity,
                  ptp_clock->portDS.portIdentity.clockIdentity,
                  CLOCK_IDENTITY_LENGTH);

    if (comp > 0)
    {
      // Reciever less than sender.
      DBGVV("PTPD: ptpd_data_set_comparison: PortIdentity: A better then B\n");
      return A_better_then_B;
    }
    else if (comp < 0)
    {
      // Receiver greater than sender.
      DBGVV("PTPD: ptpd_data_set_comparison: PortIdentity: A better by topology then B\n");
      return A_better_by_topology_then_B;
    }

    DBGV("PTPD: ptpd_data_set_comparison: ERROR 1\n");
    return ERROR_1;
  }

  comp = memcmp(header_a->sourcePortIdentity.clockIdentity,
                header_b->sourcePortIdentity.clockIdentity,
                CLOCK_IDENTITY_LENGTH);
  if (comp > 0)
  {
    // A > B
    DBGVV("PTPD: ptpd_data_set_comparison: sourcePortIdentity: B better by topology then A\n");
    return B_better_by_topology_then_A;
  }
  else if (comp < 0)
  {
    // B > A
    DBGVV("PTPD: ptpd_data_set_comparison: sourcePortIdentity: A better by topology then B\n");
    return A_better_by_topology_then_B;
  }

  // Compare port numbers of recievers of A and B - same as we have only one port.
  DBGV("PTPD: ptpd_data_set_comparison: ERROR 2\n");
  return ERROR_2;
}

// State decision algorithm 9.3.3 Fig 26.
static uint8_t ptpd_state_decision(MsgHeader *header, MsgAnnounce *announce, PtpClock *ptp_clock)
{
  int comp;

  if ((!ptp_clock->foreignMasterDS.count) && (ptp_clock->portDS.portState == PTP_LISTENING))
  {
    return PTP_LISTENING;
  }

  ptpd_copy_d0(&ptp_clock->msgTmpHeader, &ptp_clock->msgTmp.announce, ptp_clock);

  comp = ptpd_data_set_comparison(&ptp_clock->msgTmpHeader, &ptp_clock->msgTmp.announce, header, announce, ptp_clock);

  DBGV("PTPD: ptpd_state_decision: %d\n", comp);

  if (ptp_clock->defaultDS.clockQuality.clockClass < 128)
  {
    if (A_better_then_B == comp)
    {
      // M1.
      ptpd_m1(ptp_clock);
      return PTP_MASTER;
    }
    else
    {
      ptpd_p1(ptp_clock);
      return PTP_PASSIVE;
    }
  }
  else
  {
    if (A_better_then_B == comp)
    {
      // M2.
      ptpd_m2(ptp_clock);
      return PTP_MASTER;
    }
    else
    {
      ptpd_s1(ptp_clock, header, announce);
      return PTP_SLAVE;
    }
  }
}

// The best master clock (BMC) algorithm performs a distributed selection of the
// best candidate clock based on the following clock properties:
//
//  * Identifier – A universally unique numeric identifier for the clock. This is
//    typically constructed based on a device's MAC address.
//  * Quality – Both versions of IEEE 1588 attempt to quantify clock quality based
//    on expected timing deviation, technology used to implement the clock or location
//    in a stratum schema, although only V1 (IEEE 1588-2002) knows a data field stratum.
//    PTP V2 (IEEE 1588-2008) defines the overall quality of a clock by using the data
//    fields clockAccuracy and clockClass.
//  * Priority – An administratively assigned precedence hint used by the BMC to help
//    select a grandmaster for the PTP domain. IEEE 1588-2002 used a single boolean
//    variable to indicate precedence. IEEE 1588-2008 features two 8-bit priority fields.
//  * Variance – A clock's estimate of its stability based on observation of its
//    performance against the PTP reference. IEEE 1588-2008 uses a hierarchical selection
//    algorithm based on the following properties, in the indicated order:
//
//    1. Priority 1 – the user can assign a specific static-designed priority to each clock,
//       preemptively defining a priority among them. Smaller numeric values indicate
//       higher priority.
//    2. Class – each clock is a member of a given class, each class getting its own 
//       priority.
//    3. Accuracy – precision between clock and UTC, in nanoseconds (ns)
//    4. Variance – variability of the clock
//    5. Priority 2 – final-defined priority, defining backup order in case the other 
//       criteria were not sufficient. Smaller numeric values indicate higher priority.
//    6. Unique identifier – MAC address-based selection is used as a tiebreaker when
//       all other properties are equal.
//

// Compare data set of foreign masters and local data set to return the recommended
// state for the port.
uint8_t ptpd_bmc(PtpClock *ptp_clock)
{
  int16_t i;
  int16_t best;

  // Starting from i = 1, not necessery to test record[i = 0] against record[best = 0] -> they are the same.
  for (i = 1, best = 0; i < ptp_clock->foreignMasterDS.count; i++)
  {
    if ((ptpd_data_set_comparison(&ptp_clock->foreignMasterDS.records[i].header,
                                  &ptp_clock->foreignMasterDS.records[i].announce,
                                  &ptp_clock->foreignMasterDS.records[best].header,
                                  &ptp_clock->foreignMasterDS.records[best].announce, ptp_clock)) < 0)
    {
      best = i;
    }
  }

  DBGV("PTPD: ptpd_bmc: best record %d\n", best);
  ptp_clock->foreignMasterDS.best = best;

  return ptpd_state_decision(&ptp_clock->foreignMasterDS.records[best].header,
                             &ptp_clock->foreignMasterDS.records[best].announce,
                             ptp_clock);
}

#endif // LWIP_PTPD
