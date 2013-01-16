#include "../include/hubo_plus.h"

hubo_plus::hubo_plus()
{
    memset( &H_Ref,   0, sizeof(H_Ref)   );
    memset( &H_Cmd,   0, sizeof(H_Cmd)   );
    memset( &H_State, 0, sizeof(H_State) );
    memset( &H_Ctrl,  0, sizeof(H_Ctrl)  );
    memset( &H_Param, 0, sizeof(H_Param) );
    memset( &fastrak, 0, sizeof(fastrak) );

    setJointParams( &H_Param, &H_State );

    int r = ach_open( &chan_hubo_ref, HUBO_CHAN_REF_NAME, NULL );
    assert( ACH_OK == r );

    r = ach_open( &chan_hubo_state, HUBO_CHAN_STATE_NAME, NULL );
    assert( ACH_OK == r );

    r = ach_open( &chan_hubo_ctrl, HUBO_CHAN_CTRL_NAME, NULL );
    assert( ACH_OK == r );

    r = ach_open( &chan_hubo_board_cmd, HUBO_CHAN_BOARD_CMD_NAME, NULL );
    assert( ACH_OK == r );

    size_t fs;

    ach_get( &chan_hubo_ref, &H_Ref, sizeof(H_Ref), &fs, NULL, ACH_O_LAST );
    ach_get( &chan_hubo_state, &H_State, sizeof(H_State), &fs, NULL, ACH_O_LAST );
    ach_get( &chan_hubo_ctrl, &H_Ctrl, sizeof(H_Ctrl), &fs, NULL, ACH_O_LAST );


    armjoints[LEFT][0] = LSP;
    armjoints[LEFT][1] = LSR;
    armjoints[LEFT][2] = LSY;
    armjoints[LEFT][3] = LEB;
    armjoints[LEFT][4] = LWY;
    armjoints[LEFT][5] = LWP;

    armjoints[RIGHT][0] = RSP;
    armjoints[RIGHT][1] = RSR;
    armjoints[RIGHT][2] = RSY;
    armjoints[RIGHT][3] = REB;
    armjoints[RIGHT][4] = RWY;
    armjoints[RIGHT][5] = RWP;

    legjoints[LEFT][0] = LHP;
    legjoints[LEFT][1] = LHR;
    legjoints[LEFT][2] = LHY;
    legjoints[LEFT][3] = LKN;
    legjoints[LEFT][4] = LAP;
    legjoints[LEFT][5] = LAR;

    legjoints[RIGHT][0] = RHP;
    legjoints[RIGHT][1] = RHR;
    legjoints[RIGHT][2] = RHY;
    legjoints[RIGHT][3] = RKN;
    legjoints[RIGHT][4] = RAP;
    legjoints[RIGHT][5] = RAR;

    fastrakScale = 1.0;
    
}

hubo_plus::hubo_plus(const char *daemon_name)
{
    hubo_plus();

    daemonize(daemon_name);
}

double hubo_plus::getTime() { return H_State.time; }

hp_flag_t hubo_plus::update(bool printError)
{
    int r1, r2;
    size_t fs;
    r1 = ach_get( &chan_hubo_ref, &H_Ref, sizeof(H_Ref), &fs, NULL, ACH_O_LAST );
    if( ACH_OK != r1 && printError )
        fprintf( stdout, "Ach report -- Ref Channel: %s at time=%f",
			ach_result_to_string((ach_status_t)r1), getTime() );

    r2 = ach_get( &chan_hubo_state, &H_State, sizeof(H_State), &fs, NULL, ACH_O_LAST );
    if( ACH_OK != r2 && printError )
        fprintf( stdout, "Ach report -- State Channel: %s at time=%f",
			ach_result_to_string((ach_status_t)r2), getTime() );

    if( r1==ACH_OK && r2==ACH_OK )
        return SUCCESS;
    else
    {
        if( r1==ACH_OK )
            return STATE_STALE;
        else if( r2==ACH_OK )
            return REF_STALE;
        else
            return ALL_STALE;
    }
}

void hubo_plus::sendControls()
{
    int r = ach_put( &chan_hubo_ctrl, &H_Ctrl, sizeof(H_Ctrl) );
    if( r != ACH_OK ) fprintf(stderr, "Problem sending control commands: (%d) %s\n",
                                r, ach_result_to_string((ach_status_t)r));
}
void hubo_plus::sendCommands()
{
    int r = ach_put( &chan_hubo_board_cmd, &H_Cmd, sizeof(H_Cmd) );
    if( r != ACH_OK ) fprintf(stderr, "Problem sending board commands: (%d) %s\n",
                                r, ach_result_to_string((ach_status_t)r));
}

// ~~~*** Sending Control Commands ***~~~ //
// ~~** Setting reference values

// ~* General sets
hp_flag_t hubo_plus::resetJointStatus( int joint, bool send )
{
    if( joint < HUBO_JOINT_COUNT )
        H_Ctrl.joint[joint].mode = CTRL_RESET;
    else
        return JOINT_OOB;

    if( send )
        sendControls();

    return SUCCESS;
}
// Position control
hp_flag_t hubo_plus::setPositionControl(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
    {
        H_Ctrl.joint[joint].mode = CTRL_POS;
        H_Ctrl.joint[joint].position = H_State.joint[joint].pos;
        H_Ctrl.active=1;
    }
    else
        return JOINT_OOB;

    return SUCCESS;
}

hp_flag_t hubo_plus::setJointAngle(int joint, double radians, bool send)
{
    if( joint < HUBO_JOINT_COUNT )
    {
        H_Ctrl.joint[joint].position = radians;
        H_Ctrl.joint[joint].mode = CTRL_POS;
        H_Ctrl.active=1;
        if(send)
            sendControls();
    }
    else
        return JOINT_OOB;

    return SUCCESS;
}

hp_flag_t hubo_plus::setJointNominalSpeed(int joint, double speed)
{
    if( joint < HUBO_JOINT_COUNT )
    {
        if( H_Ctrl.joint[joint].mode == CTRL_POS )
            H_Ctrl.joint[joint].velocity = speed;
        else
            return WRONG_MODE;
    }
    else
        return JOINT_OOB;

    return SUCCESS;
}

// Velocity control
hp_flag_t hubo_plus::setVelocityControl( int joint )
{
    if( joint < HUBO_JOINT_COUNT )
    {
        H_Ctrl.joint[joint].mode = CTRL_VEL;
        H_Ctrl.joint[joint].velocity = 0;
    }
    else
        return JOINT_OOB;

    return SUCCESS;
}

hp_flag_t hubo_plus::setJointVelocity(int joint, double vel, bool send)
{
    if( joint < HUBO_JOINT_COUNT )
    {
        H_Ctrl.joint[joint].velocity = vel;
        H_Ctrl.joint[joint].mode = CTRL_VEL;
        H_Ctrl.active=1;
        if(send)
            sendControls();
    }
    else
        return JOINT_OOB;

    return SUCCESS;
}

// Acceleration setting
hp_flag_t hubo_plus::setJointNominalAcceleration(int joint, double acc)
{
    if( joint < HUBO_JOINT_COUNT )
        H_Ctrl.joint[joint].acceleration = acc;
    else
        return JOINT_OOB;

    return SUCCESS;
}


// ~* Arm control sets
// Position control
hp_flag_t hubo_plus::setArmPosCtrl(int side)
{
    if( side==LEFT || side==RIGHT )
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            setPositionControl(armjoints[side][i]);
    else
        return BAD_SIDE;

    return SUCCESS;
}

hp_flag_t hubo_plus::setArmAngles(int side, Vector6d angles, bool send)
{
    H_Ctrl.active=1;
    if( angles.size() < ARM_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( angles.size() > ARM_JOINT_COUNT )
        return LONG_VECTOR;


    if( side==LEFT || side==RIGHT )
       for(int i=0; i<ARM_JOINT_COUNT; i++)
            setJointAngle(armjoints[side][i], angles[i], false);

    else
        return BAD_SIDE;

    if(send)
        sendControls();

    return SUCCESS;
}

void hubo_plus::setLeftArmPosCtrl() { setArmPosCtrl(LEFT); }

hp_flag_t hubo_plus::setLeftArmAngles(Vector6d angles, bool send)
{ return setArmAngles( LEFT, angles, send ); }

void hubo_plus::setRightArmPosCtrl() { setArmPosCtrl(RIGHT); }

hp_flag_t hubo_plus::setRightArmAngles(Vector6d angles, bool send)
{ return setArmAngles( RIGHT, angles, send ); }


hp_flag_t hubo_plus::setArmNomSpeeds(int side, Vector6d speeds)
{
    if( speeds.size() < ARM_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( speeds.size() > ARM_JOINT_COUNT )
        return LONG_VECTOR;


    if( side==LEFT || side==RIGHT )
    {
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            if( H_Ctrl.joint[armjoints[side][i]].mode != CTRL_POS )
                return WRONG_MODE;

        for(int i=0; i<ARM_JOINT_COUNT; i++)
            setJointNominalSpeed( armjoints[side][i], speeds(i) );
    }
    else
        return BAD_SIDE;


    return SUCCESS;
}

hp_flag_t hubo_plus::setLeftArmNomSpeeds(Vector6d speeds)
{ return setArmNomSpeeds(LEFT, speeds); }

hp_flag_t hubo_plus::setRightArmNomSpeeds(Vector6d speeds)
{ return setArmNomSpeeds(RIGHT, speeds); }


// Velocity Control
hp_flag_t hubo_plus::setArmVelCtrl(int side)
{
    if( side==LEFT || side==RIGHT )
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            setVelocityControl( armjoints[side][i] );
    else
        return BAD_SIDE;

    return SUCCESS;
}

hp_flag_t hubo_plus::setArmVels(int side, Vector6d vels, bool send)
{
    H_Ctrl.active=1;
    if( vels.size() < ARM_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( vels.size() > ARM_JOINT_COUNT )
        return LONG_VECTOR;


    if( side==LEFT || side==RIGHT )
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            setJointVelocity(armjoints[side][i], vels(i), false);
    else
        return BAD_SIDE;

    if(send)
        sendControls();

    return SUCCESS;
}

void hubo_plus::setLeftArmVelCtrl() { setArmVelCtrl(LEFT); }

hp_flag_t hubo_plus::setLeftArmVels(Vector6d vels, bool send)
{ return setArmVels(LEFT, vels, send); }

void hubo_plus::setRightArmVelCtrl() { setArmVelCtrl(RIGHT); }

hp_flag_t hubo_plus::setRightArmVels(Vector6d vels, bool send)
{ return setArmVels(RIGHT, vels, send); }


// Acceleration settings
hp_flag_t hubo_plus::setArmNomAcc(int side, Vector6d acc)
{
    if( acc.size() < ARM_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( acc.size() > ARM_JOINT_COUNT )
        return LONG_VECTOR;

    if( side==LEFT || side==RIGHT )
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            setJointNominalAcceleration( armjoints[side][i], acc(i) );
    else
        return BAD_SIDE;

    return SUCCESS;
}

hp_flag_t hubo_plus::setLeftArmNomAcc(Vector6d acc)
{ return setArmNomAcc( LEFT, acc ); }

hp_flag_t hubo_plus::setRightArmNomAcc(Vector6d acc)
{ return setArmNomAcc( RIGHT, acc ); }


// ~* Leg control sets
// Position control
hp_flag_t hubo_plus::setLegPosCtrl(int side)
{
    if( side==LEFT || side==RIGHT )
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            setPositionControl(legjoints[side][i]);
    else
        return BAD_SIDE;

    return SUCCESS;
}

hp_flag_t hubo_plus::setLegAngles(int side, Vector6d angles, bool send)
{
    if( angles.size() < LEG_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( angles.size() > LEG_JOINT_COUNT )
        return LONG_VECTOR;


    if( side==LEFT || side==RIGHT )
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            setJointAngle(legjoints[side][i], angles[i], false);
    else
        return BAD_SIDE;


    if(send)
        sendControls();

    return SUCCESS;
}

void hubo_plus::setLeftLegPosCtrl() { setLegPosCtrl(LEFT); }

hp_flag_t hubo_plus::setLeftLegAngles(Vector6d angles, bool send)
{ return setLegAngles( LEFT, angles, send ); }

void hubo_plus::setRightLegPosCtrl() { setLegPosCtrl(RIGHT); }

hp_flag_t hubo_plus::setRightLegAngles(Vector6d angles, bool send)
{ return setLegAngles( RIGHT, angles, send ); }


hp_flag_t hubo_plus::setLegNomSpeeds(int side, Vector6d speeds)
{
    if( speeds.size() < LEG_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( speeds.size() > LEG_JOINT_COUNT )
        return LONG_VECTOR;


    if( side==LEFT || side==RIGHT )
    {
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            if( H_Ctrl.joint[legjoints[side][i]].mode != CTRL_POS )
                return WRONG_MODE;

        for(int i=0; i<LEG_JOINT_COUNT; i++)
            setJointNominalSpeed( legjoints[side][i], speeds(i) );
    }
    else
        return BAD_SIDE;


    return SUCCESS;
}

hp_flag_t hubo_plus::setLeftLegNomSpeeds(Vector6d speeds)
{ return setLegNomSpeeds(LEFT, speeds); }

hp_flag_t hubo_plus::setRightLegNomSpeeds(Vector6d speeds)
{ return setLegNomSpeeds(RIGHT, speeds); }


// Velocity Control
hp_flag_t hubo_plus::setLegVelCtrl(int side)
{
    if( side==LEFT || side==RIGHT )
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            setVelocityControl( legjoints[side][i] );
    else
        return BAD_SIDE;

    return SUCCESS;
}

hp_flag_t hubo_plus::setLegVels(int side, Vector6d vels, bool send)
{
    H_Ctrl.active=1;
    if( vels.size() < LEG_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( vels.size() > LEG_JOINT_COUNT )
        return LONG_VECTOR;


    if( side==LEFT || side==RIGHT )
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            setJointVelocity(legjoints[side][i], vels(i), false);
    else
        return BAD_SIDE;

    if(send)
        sendControls();

    return SUCCESS;
}

void hubo_plus::setLeftLegVelCtrl() { setLegVelCtrl(LEFT); }

hp_flag_t hubo_plus::setLeftLegVels(Vector6d vels, bool send)
{ return setLegVels(LEFT, vels, send); }

void hubo_plus::setRightLegVelCtrl() { setLegVelCtrl(RIGHT); }

hp_flag_t hubo_plus::setRightLegVels(Vector6d vels, bool send)
{ return setLegVels(RIGHT, vels, send); }


// Acceleration settings
hp_flag_t hubo_plus::setLegNomAcc(int side, Vector6d acc)
{
    if( acc.size() < LEG_JOINT_COUNT )
        return SHORT_VECTOR;
    else if( acc.size() > LEG_JOINT_COUNT )
        return LONG_VECTOR;

    if( side==LEFT || side==RIGHT )
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            setJointNominalAcceleration( legjoints[side][i], acc(i) );
    else
        return BAD_SIDE;

    return SUCCESS;
}

hp_flag_t hubo_plus::setLeftLegNomAcc(Vector6d acc)
{ return setLegNomAcc( LEFT, acc ); }

hp_flag_t hubo_plus::setRightLegNomAcc(Vector6d acc)
{ return setLegNomAcc( RIGHT, acc ); }


// ~~** Setting limit values
// ~* General sets
hp_flag_t hubo_plus::setJointAngleMin(int joint, double radians)
{
    if( joint < HUBO_JOINT_COUNT )
        H_Ctrl.joint[joint].pos_min = radians;
    else
        return JOINT_OOB;

    return SUCCESS;
}

hp_flag_t hubo_plus::setJointAngleMax(int joint, double radians)
{
    if( joint < HUBO_JOINT_COUNT )
        H_Ctrl.joint[joint].pos_max = radians;
    else
        return JOINT_OOB;

    return SUCCESS;
}

hp_flag_t hubo_plus::setJointSpeedMax(int joint, double speed)
{
    if( joint < HUBO_JOINT_COUNT )
        H_Ctrl.joint[joint].speed_limit = speed;
    else
        return JOINT_OOB;

    return SUCCESS;
}

hp_flag_t hubo_plus::setJointAccelMax(int joint, double accel)
{
    if( joint < HUBO_JOINT_COUNT )
        H_Ctrl.joint[joint].accel_limit = accel;
    else
        return JOINT_OOB;

    return SUCCESS;
}



// ~~** Getting Reference Values
// ~* General gets
// Position control
hubo_ctrl_mode_t hubo_plus::getCtrlMode(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].mode;
    else
        return CTRL_OFF;
}

double hubo_plus::getJointAngle(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].position;
    else
        return 0;
}

double hubo_plus::getJointNominalSpeed(int joint)
{ return getJointVelocity(joint); }

// Velocity control
double hubo_plus::getJointVelocity(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].velocity;
    else
        return 0;
}

// Acceleration setting
double hubo_plus::getJointNominalAcceleration(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].acceleration;
    else
        return 0;
}

int hubo_plus::getJointStatus( int joint )
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ref.status[joint];
    else
        return 0;
}

// ~* Arm control gets
// Position control
hp_flag_t hubo_plus::getArmAngles(int side, Vector6d &angles)
{
    if( side==LEFT || side==RIGHT )
    {
        if(angles.size() != ARM_JOINT_COUNT)
            angles.resize(ARM_JOINT_COUNT);
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            angles[i] = H_Ctrl.joint[armjoints[side][i]].position;
    }
    else
        return BAD_SIDE;

    return SUCCESS;
}
void hubo_plus::getLeftArmAngles(Vector6d &angles)
{ getArmAngles(LEFT, angles); }
void hubo_plus::getRightArmAngles(Vector6d &angles)
{ getArmAngles(RIGHT, angles); }

hp_flag_t hubo_plus::getArmNomSpeeds(int side, Vector6d &speeds)
{ return getArmVels(side, speeds); }
void hubo_plus::getLeftArmNomSpeeds(Vector6d &speeds)
{ getArmVels(LEFT, speeds); }
void hubo_plus::getRightArmNomSpeeds(Vector6d &speeds)
{ getArmVels(RIGHT, speeds); }

// Velocity control
hp_flag_t hubo_plus::getArmVels(int side, Vector6d &vels)
{
    if( side==LEFT || side==RIGHT )
    {
        if(vels.size() != ARM_JOINT_COUNT)
            vels.resize(ARM_JOINT_COUNT);

        for(int i=0; i<ARM_JOINT_COUNT; i++)
            vels[i] = H_Ctrl.joint[armjoints[side][i]].velocity;
    }
    else
        return BAD_SIDE;

    return SUCCESS;
}
void hubo_plus::getLeftArmVels(Vector6d &vels)
{ getArmVels(LEFT, vels); }
void hubo_plus::getRightArmVels(Vector6d &vels)
{ getArmVels(RIGHT, vels); }

// Acceleration settings
hp_flag_t hubo_plus::getArmNomAcc(int side, Vector6d &acc)
{
    if( side==LEFT || side==RIGHT )
    {
        if(acc.size() != ARM_JOINT_COUNT)
            acc.resize(ARM_JOINT_COUNT);

        for(int i=0; i<ARM_JOINT_COUNT; i++)
            acc[i] = H_Ctrl.joint[armjoints[side][i]].acceleration;
    }
    else
        return BAD_SIDE;
}
void hubo_plus::getLeftArmNomAcc(Vector6d &acc)
{ getArmNomAcc(LEFT, acc); }
void hubo_plus::getRightArmNomAcc(Vector6d &acc)
{ getArmNomAcc(RIGHT, acc); }


// ~* Leg control gets
// Position control
hp_flag_t hubo_plus::getLegAngles(int side, Vector6d &angles)
{
    if( side==LEFT || side==RIGHT )
    {
        if(angles.size() != LEG_JOINT_COUNT)
            angles.resize(LEG_JOINT_COUNT);
        for(int i=0; i<LEG_JOINT_COUNT; i++)
            angles[i] = H_Ctrl.joint[legjoints[side][i]].position;
    }
    else
        return BAD_SIDE;

    return SUCCESS;
}
void hubo_plus::getLeftLegAngles(Vector6d &angles)
{ getLegAngles(LEFT, angles); }
void hubo_plus::getRightLegAngles(Vector6d &angles)
{ getLegAngles(RIGHT, angles); }

hp_flag_t hubo_plus::getLegNomSpeeds(int side, Vector6d &speeds)
{ return getLegVels(side, speeds); }
void hubo_plus::getLeftLegNomSpeeds(Vector6d &speeds)
{ getLegVels(LEFT, speeds); }
void hubo_plus::getRightLegNomSpeeds(Vector6d &speeds)
{ getLegVels(RIGHT, speeds); }

// Velocity control
hp_flag_t hubo_plus::getLegVels(int side, Vector6d &vels)
{
    if( side==LEFT || side==RIGHT )
    {
        if(vels.size() != LEG_JOINT_COUNT)
            vels.resize(LEG_JOINT_COUNT);

        for(int i=0; i<LEG_JOINT_COUNT; i++)
            vels[i] = H_Ctrl.joint[legjoints[side][i]].velocity;
    }
    else
        return BAD_SIDE;

    return SUCCESS;
}
void hubo_plus::getLeftLegVels(Vector6d &vels)
{ getLegVels(LEFT, vels); }
void hubo_plus::getRightLegVels(Vector6d &vels)
{ getLegVels(RIGHT, vels); }

// Acceleration settings
hp_flag_t hubo_plus::getLegNomAcc(int side, Vector6d &acc)
{
    if( side==LEFT || side==RIGHT )
    {
        if(acc.size() != LEG_JOINT_COUNT)
            acc.resize(LEG_JOINT_COUNT);

        for(int i=0; i<LEG_JOINT_COUNT; i++)
            acc[i] = H_Ctrl.joint[legjoints[side][i]].acceleration;
    }
    else
        return BAD_SIDE;
}
void hubo_plus::getLeftLegNomAcc(Vector6d &acc)
{ getLegNomAcc(LEFT, acc); }
void hubo_plus::getRightLegNomAcc(Vector6d &acc)
{ getLegNomAcc(RIGHT, acc); }


// ~~** Getting limit values
// ~* General gets
double hubo_plus::getJointAngleMin( int joint )
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].pos_min;
    else
        return 0;
}

double hubo_plus::getJointAngleMax(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].pos_max;
    else
        return 0;
}

double hubo_plus::getJointSpeedMax(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].speed_limit;
    else
        return 0;
}

double hubo_plus::getJointAccelMax(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_Ctrl.joint[joint].accel_limit;
    else
        return 0;
}



// ~~~*** State Readings ***~~~ //

// ~~** State
// TODO: Stuff like state position, velocity, whatever
double hubo_plus::getJointAngleState(int joint)
{
    if( joint < HUBO_JOINT_COUNT )
        return H_State.joint[joint].pos;
    else
        return 0;
}

hp_flag_t hubo_plus::getArmAngleStates( int side, Vector6d &angles )
{
    if( side==LEFT || side==RIGHT )
        for(int i=0; i<ARM_JOINT_COUNT; i++)
            angles[i] = getJointAngleState( armjoints[side][i] );
    else
        return BAD_SIDE;

    return SUCCESS;
} 
void hubo_plus::getRightArmAngleStates( Vector6d &angles )
{ getArmAngleStates( RIGHT, angles ); }
void hubo_plus::getLeftArmAngleStates( Vector6d &angles )
{ getArmAngleStates( LEFT, angles ); }


// ~~** Sensors
// ~* Force-torque
// Mx
double hubo_plus::getMx(hubo_ft_index_t sensor)
{
    if( sensor==HUBO_FT_R_FOOT || sensor==HUBO_FT_R_HAND ||
        sensor==HUBO_FT_L_FOOT || sensor==HUBO_FT_L_HAND )
        return H_State.ft[sensor].m_x;
    else
        return 0;
}
double hubo_plus::getRightHandMx() { getMx(HUBO_FT_R_HAND); }
double hubo_plus::getLeftHandMx()  { getMx(HUBO_FT_L_HAND); }
double hubo_plus::getRightFootMx() { getMx(HUBO_FT_R_FOOT); }
double hubo_plus::getLeftFootMx()  { getMx(HUBO_FT_L_FOOT); }

// My
double hubo_plus::getMy(hubo_ft_index_t sensor)
{
    if( sensor==HUBO_FT_R_FOOT || sensor==HUBO_FT_R_HAND ||
        sensor==HUBO_FT_L_FOOT || sensor==HUBO_FT_L_HAND )
        return H_State.ft[sensor].m_y;
    else
        return 0;
}
double hubo_plus::getRightHandMy() { getMy(HUBO_FT_R_HAND); }
double hubo_plus::getLeftHandMy()  { getMy(HUBO_FT_L_HAND); }
double hubo_plus::getRightFootMy() { getMy(HUBO_FT_R_FOOT); }
double hubo_plus::getLeftFootMy()  { getMy(HUBO_FT_L_FOOT); }

// Fz
double hubo_plus::getFz(hubo_ft_index_t sensor)
{
    if( sensor==HUBO_FT_R_FOOT || sensor==HUBO_FT_L_FOOT )
        return H_State.ft[sensor].f_z;
    else
        return 0;
}
double hubo_plus::getRightFootFz() { getFz(HUBO_FT_R_FOOT); }
double hubo_plus::getLeftFootFz()  { getFz(HUBO_FT_L_FOOT); }


// ~* Accelerometers
// AccX
double hubo_plus::getAccX(int side)
{
    if( side==LEFT || side==RIGHT )
        return H_State.imu.a_foot_x[side];
    else
        return 0;
}
double hubo_plus::getLeftAccX() { return getAccX(LEFT); }
double hubo_plus::getRightAccX() { return getAccX(RIGHT); }

// AccY
double hubo_plus::getAccY(int side)
{
    if( side==LEFT || side==RIGHT )
        return H_State.imu.a_foot_y[side];
    else
        return 0;
}
double hubo_plus::getLeftAccY() { return getAccY(LEFT); }
double hubo_plus::getRightAccY() { return getAccY(RIGHT); }

// AccZ
double hubo_plus::getAccZ(int side)
{
    if( side==LEFT || side==RIGHT )
        return H_State.imu.a_foot_z[side];
    else
        return 0;
}
double hubo_plus::getLeftAccZ() { return getAccZ(LEFT); }
double hubo_plus::getRightAccZ() { return getAccZ(RIGHT); }

// ~* IMU
double hubo_plus::getAngleX() { return H_State.imu.angle_x; }
double hubo_plus::getAngleY() { return H_State.imu.angle_y; }
double hubo_plus::getRotVelX() { return H_State.imu.w_x; }
double hubo_plus::getRotVelY() { return H_State.imu.w_y; }



// ~~~*** Board Commands ***~~~ //
hp_flag_t hubo_plus::homeJoint( int joint, bool send, double wait )
{
    if( joint < HUBO_JOINT_COUNT )
    {
        H_Ctrl.joint[joint].position = 0;
        H_Ctrl.joint[joint].mode = CTRL_HOME;
        H_Ctrl.active = 2;
        
        H_Cmd.type = D_GOTO_HOME;
        H_Cmd.joint = joint;
    }
    else
        return JOINT_OOB;

    if(send)
    {
        sendControls();
        while( H_Ref.paused==0 )
            update();
        sendCommands();
    }

    return SUCCESS;
}

void hubo_plus::homeAllJoints( bool send, double wait )
{
    for(int i=0; i<HUBO_JOINT_COUNT; i++)
        homeJoint( i, false );
    
    H_Ctrl.active = 2;

    H_Cmd.type = D_GOTO_HOME_ALL;

    if(send)
    {
        sendControls();
        while( H_Ref.paused==0 )
            update();
        sendCommands();
    }
}





// ~~~*** Kinematics ***~~~ //
inline double min(double x, double y) { return ( x > y ) ? y : x; }
inline double max(double x, double y) { return ( x < y ) ? y : x; }

void hubo_plus::DH2HG(Eigen::Isometry3d &B, double t, double f, double r, double d)
{
    // Convert DH parameters (standard convention) to Homogenuous transformation matrix.
    B = Eigen::Isometry3d::Identity();
    
    B.translate(Eigen::Vector3d(0.,0.,d));
    B.rotate(Eigen::AngleAxisd(t, Eigen::Vector3d::UnitZ()));
    B.translate(Eigen::Vector3d(r,0,0));
    B.rotate(Eigen::AngleAxisd(f, Eigen::Vector3d::UnitX()));
    
}


void hubo_plus::huboArmFK(Eigen::Isometry3d &B, Vector6d &q, int side) {
    Eigen::Isometry3d hand;
    hand(0,0) =  1; hand(0,1) =  0; hand(0,2) = 0; hand(0,3) =   0;
    hand(1,0) =  0; hand(1,1) =  0; hand(1,2) =-1; hand(1,3) =   0;
    hand(2,0) =  0; hand(2,1) =  1; hand(2,2) = 0; hand(2,3) =   0;
    hand(3,0) =  0; hand(3,1) =  0; hand(3,2) = 0; hand(3,3) =   1;
    
    huboArmFK(B, q, side, hand);
}

void hubo_plus::huboArmFK(Eigen::Isometry3d &B, Vector6d &q, int side,  const Eigen::Isometry3d &endEffector)
{
    // Declarations
    Eigen::Isometry3d neck, hand, T;
    Eigen::MatrixXd limits(6,2);
    Vector6d offset; offset.setZero();
    
    // Parameters
    double l1 = 214.5/1000.0;
    double l2 = 179.14/1000.0;
    double l3 = 181.59/1000.0;
    double l4 = 50.0/1000.0;
    
    Vector6d t, f, r, d;
    t <<  M_PI/2, -M_PI/2,  M_PI/2,       0,       0,  M_PI/2;
    f <<  M_PI/2,  M_PI/2, -M_PI/2,  M_PI/2, -M_PI/2,       0;
    r <<       0,       0,       0,       0,       0,      l4;
    d <<       0,       0,     -l2,       0,     -l3,       0;
    
    if (side == RIGHT) {
        neck(0,0) = 1; neck(0,1) =  0; neck(0,2) = 0; neck(0,3) =   0;
        neck(1,0) = 0; neck(1,1) =  0; neck(1,2) = 1; neck(1,3) = -l1;
        neck(2,0) = 0; neck(2,1) = -1; neck(2,2) = 0; neck(2,3) =   0;
        neck(3,0) = 0; neck(3,1) =  0; neck(3,2) = 0; neck(3,3) =   1;
        
        limits <<
        -2,   2,
        -2,  .3,
        -2,   2,
        -2,   0.01,
        -2,   2,
        -1.4, 1.2;
        
        // Set offsets
        offset(1) = limits(1,1); // Note: I think this might be backwards
//        offset(1) = -limits(1,1);
        
    } else {
        neck(0,0) = 1; neck(0,1) =  0; neck(0,2) = 0; neck(0,3) =   0;
        neck(1,0) = 0; neck(1,1) =  0; neck(1,2) = 1; neck(1,3) =  l1;
        neck(2,0) = 0; neck(2,1) = -1; neck(2,2) = 0; neck(2,3) =   0;
        neck(3,0) = 0; neck(3,1) =  0; neck(3,2) = 0; neck(3,3) =   1;
        
        limits <<
        -2,   2,
        -.3,   2,
        -2,   2,
        -2,   0.01,
        -2,   2,
        -1.4, 1.2;
        
        // Set offsets
        offset(1) = limits(1,0); // Note: I think this might be backwards
//        offset(1) = -limits(1,0);
    }
     
    // Calculate forward kinematics
    B = neck;
    for (int i = 0; i < 6; i++) {
        DH2HG(T, t(i)+q(i)-offset(i), f(i), r(i), d(i));
        B = B*T;
    }
    B = B*endEffector;
    
}


void hubo_plus::huboArmIK(Vector6d &q, Eigen::Isometry3d B, Vector6d qPrev, int side) {
    // Hand	
    Eigen::Isometry3d endEffector;
    endEffector(0,0) =  1; endEffector(0,1) =  0; endEffector(0,2) = 0; endEffector(0,3) =   0;
    endEffector(1,0) =  0; endEffector(1,1) =  0; endEffector(1,2) =-1; endEffector(1,3) =   0;
    endEffector(2,0) =  0; endEffector(2,1) =  1; endEffector(2,2) = 0; endEffector(2,3) =   0;
    endEffector(3,0) =  0; endEffector(3,1) =  0; endEffector(3,2) = 0; endEffector(3,3) =   1;
    
    huboArmIK(q, B, qPrev, side, endEffector);
}
  
void hubo_plus::huboArmIK(Vector6d &q, Eigen::Isometry3d B, Vector6d qPrev, int side, const Eigen::Isometry3d &endEffector)
{
    
    Eigen::ArrayXXd qAll(6,8);
    
    // Declarations
    Eigen::Isometry3d neck, neckInv, endEffectorInv, BInv;
    Eigen::MatrixXd limits(6,2);
    Vector6d offset; offset.setZero();
    double nx, sx, ax, px;
    double ny, sy, ay, py;
    double nz, sz, az, pz;
    double q1, q2, q3, q4, q5, q6;
    double qP1, qP3;
    double qT;
    Eigen::Matrix<int, 8, 3> m;
    
    double S2, S4, S5, S6;
    double C2, C4, C5, C6;
    
    // Parameters
    double l1 = 214.5/1000.0;
    double l2 = 179.14/1000.0;
    double l3 = 181.59/1000.0;
    double l4 = 50.0/1000.0;
    
    if (side == RIGHT) {
        neck(0,0) = 1; neck(0,1) =  0; neck(0,2) = 0; neck(0,3) =   0;
        neck(1,0) = 0; neck(1,1) =  0; neck(1,2) = 1; neck(1,3) = -l1;
        neck(2,0) = 0; neck(2,1) = -1; neck(2,2) = 0; neck(2,3) =   0;
        neck(3,0) = 0; neck(3,1) =  0; neck(3,2) = 0; neck(3,3) =   1;
        
        limits <<
        -2,   2,
        -2,  .3,
        -2,   2,
        -2,   0.01,
        -2,   2,
        -1.4, 1.2;
        
        // Set offsets
        offset(1) = limits(1,1); // Note: I think this is backwards
//        offset(1) = -limits(1,1);
        
    } else {
        neck(0,0) = 1; neck(0,1) =  0; neck(0,2) = 0; neck(0,3) =   0;
        neck(1,0) = 0; neck(1,1) =  0; neck(1,2) = 1; neck(1,3) =  l1;
        neck(2,0) = 0; neck(2,1) = -1; neck(2,2) = 0; neck(2,3) =   0;
        neck(3,0) = 0; neck(3,1) =  0; neck(3,2) = 0; neck(3,3) =   1;
        
        limits <<
        -2,   2,
        -.3,   2,
        -2,   2,
        -2,   0.01,
        -2,   2,
        -1.4, 1.2;
        
        // Set offsets
        offset(1) = limits(1,0); // Note: I think this is backwards
//        offset(1) = -limits(1,0);
    }
    neckInv = neck.inverse();
    
    endEffectorInv = endEffector.inverse();        

    double zeroSize = .000001;
    
    // Variables
    B = neckInv*B*endEffectorInv;
    BInv = B.inverse();
    
    nx = BInv(0,0); sx = BInv(0,1); ax = BInv(0,2); px = BInv(0,3);
    ny = BInv(1,0); sy = BInv(1,1); ay = BInv(1,2); py = BInv(1,3);
    nz = BInv(2,0); sz = BInv(2,1); az = BInv(2,2); pz = BInv(2,3);

    
    qP1 = qPrev(0); qP3 = qPrev(2);
    
    m <<
    1,  1,  1,
    1,  1, -1,
    1, -1,  1,
    1, -1, -1,
    -1,  1,  1,
    -1,  1, -1,
    -1, -1,  1,
    -1, -1, -1;
    
    // Calculate inverse kinematics
    for (int i = 0; i < 8; i++) {
        
        // Solve for q4
        C4 = max(min((2*l4*px - l2*l2 - l3*l3 + l4*l4 + px*px + py*py + pz*pz)/(2*l2*l3),1),-1);
        if (fabs(C4 - 1) < zeroSize) { // Case 1: q4 == 0
            
            // Set q4
            q4 = 0;
            
            // Set q3
            q3 = qP3;
            
            // Solve for q6
            S6 = max(min( py/(l2 + l3), 1),-1);
            C6 = max(min( -(l4 + px)/(l2 + l3), 1), -1);
            q6 = atan2(S6,C6);
            

            // Solve for q2
            S2 = max(min( C4*C6*ax - C4*S6*ay, 1),-1);
            if (fabs(S2 - 1) < zeroSize) {
                q2 = M_PI/2;
            } else if (fabs(S2 + 1) < zeroSize) {
                q2 = -M_PI/2;
            } else {
                double complex radical = 1-S2*S2;
                q2 = atan2(S2,m(i,2)*creal(csqrt(radical)));
            }
            
            // Solve for q5
            qT = atan2(-C6*ay - S6*ax,az);
            C2 = cos(q2);
            
            if (fabs(C2) < zeroSize) { // Case 3: q2 = pi/2 or -pi/2
                
                q1 = qP1;
                q3 = qP3;
                
                // Solve for q5
                if (S2 > 0) { // Case 3a: q2 = pi/2
                    qT = atan2(nz,-sz);
                    q5 = wrapToPi(q1 - q3 - qT);
                } else { // Case 3b: q2 = -pi/2
                    qT = atan2(-nz,sz);
                    q5 = wrapToPi(qT - q1 - q3);
                }
                
                
            } else {
                
                if (C2 < 0) {
                    qT = qT + M_PI;
                }
                q5 = wrapToPi(qT - q3);
                
                // Solve for q1
                q1 = atan2(S6*ny - C6*nx,C6*sx - S6*sy);
                if (C2 < 0) {
                    q1 = q1 + M_PI;
                }
                q1 = wrapToPi(q1);
            }
            
        } else {
            
            // Solve for q4
            double complex radical = 1-C4*C4;
            q4 = atan2(m(i,0)*creal(csqrt(radical)),C4);
            
            // Solve for q5
            S4 = sin(q4);
            S5 = pz/(S4*l2);
            if (fabs(S5 - 1) < zeroSize) {
                q5 = M_PI/2;
            } else if (fabs(S5 + 1) < zeroSize) {
                q5 = -M_PI/2;
            } else {
                radical = 1-S5*S5;
                q5 = atan2(S5,m(i,1)*creal(csqrt(radical)));
            }
            
            // Solve for q6
            C5 = cos(q5);
            S6 =max(min( (C5*S4*l2 + (py*(l3 + C4*l2 - (C5*S4*l2*py)/(l4 + px)))/(l4 + px + py*py/(l4 + px)))/(l4 + px), 1),-1);
            C6 = max(min( -(l3 + C4*l2 - (C5*S4*l2*py)/(l4 + px))/(l4 + px + py*py/(l4 + px)), 1),-1);
            q6 = atan2(S6,C6);
            
            // Solve for q2
            S2 = max(min(ax*(C4*C6 - C5*S4*S6) - ay*(C4*S6 + C5*C6*S4) - S4*S5*az,1),-1);
            if (fabs(S2 - 1) < zeroSize) {
                q2 = M_PI/2;
            } else if (fabs(S2 + 1) < zeroSize) {
                q2 = -M_PI/2;
            } else {
                radical = 1-S2*S2;
                q2 = atan2(S2,m(i,2)*creal(csqrt(radical)));
            }
            
            // Solve for q3
            C2 = cos(q2);
            
            if (fabs(C2) < zeroSize) { // Case 2: q2 = pi/2 or -pi/2
                
                q3 = qP3;
                // Solve for q1
                if (S2 > 0) { // Case 2a: q2 = pi/2
                    qT = atan2(S6*sy - C6*sx,S6*ny - C6*nx);
                    if (S4 < 0) {
                        qT = qT + M_PI;
                    }
                    q1 = wrapToPi(qT + q3);
                } else { // Case 2b: q2 = -pi/2
                    qT = atan2(S6*sy - C6*sx,S6*ny - C6*nx);
                    if (S4 < 0) {
                        qT = qT + M_PI;
                    }
                    q1 = wrapToPi(qT - q3);
                }
                
            } else {
                q3 = atan2(S4*S6*ay - C4*S5*az - C6*S4*ax - C4*C5*C6*ay - C4*C5*S6*ax,C5*az - C6*S5*ay - S5*S6*ax);
                if (C2 < 0) {
                    q3 = q3 - M_PI;
                }
                q3 = wrapToPi(q3);
                
                // Solve for q1
                q1 = atan2(C4*S6*ny - C4*C6*nx + S4*S5*nz + C5*C6*S4*ny + C5*S4*S6*nx,C4*C6*sx - C4*S6*sy - S4*S5*sz - C5*C6*S4*sy - C5*S4*S6*sx);
                if (C2 < 0) {
                    q1 = q1 + M_PI;
                }
                q1 = wrapToPi(q1);
            }
        }
        
        qAll(0,i) = q1;
        qAll(1,i) = q2;
        qAll(2,i) = q3;
        qAll(3,i) = q4;
        qAll(4,i) = q5;
        qAll(5,i) = q6;
    }
    
    // Find best solution
    Eigen::ArrayXd qDiff(6,1); qDiff.setZero();
    Eigen::ArrayXd qDiffSum(8,1);
    int minInd;
    
    for (int i = 0; i < 8; i++) {
        for (int j=0; j < 6; j++)
            qDiff(j) = wrapToPi(qAll(j,i) - qPrev(j));
        qDiffSum(i) = qDiff.abs().sum();
    }
    qDiffSum.minCoeff(&minInd);
    
    q = qAll.col(minInd);
    
    // Set to offsets
    for (int i = 0; i < 6; i++) {
        if (side==RIGHT) {
            q(i) = wrapToPi(q(i) + offset(i));
        } else {
            q(i) = wrapToPi(q(i) + offset(i));
        }
    }
    
    q = q.cwiseMin(limits.col(1)); //TODO: Put these back
    q = q.cwiseMax(limits.col(0));
}


void hubo_plus::huboLegFK(Eigen::Isometry3d &B, Vector6d &q, int side) {
    // Declarations
    Eigen::Isometry3d neck, waist, T;
    Eigen::MatrixXd limits(6,2);
    Vector6d offset; offset.setZero();
    
    // Parameters
    double l1 = (79.5+107)/1000.0;
    double l2 = 88.43/1000.0;
    double l3 = (289.47-107)/1000.0;
    double l4 = 300.03/1000.0;
    double l5 = 300.38/1000.0;
    double l6 = 94.97/1000.0;
    
    Vector6d t, f, r, d;
    t <<       0, -M_PI/2,       0,       0,       0,       0;
    f <<  M_PI/2, -M_PI/2,       0,       0,  M_PI/2,       0;
    r <<       0,       0,      l4,      l5,       0,      l6;
    d <<       0,       0,       0,       0,       0,       0;
    
    neck(0,0) = 1; neck(0,1) =  0; neck(0,2) = 0; neck(0,3) =   0;
    neck(1,0) = 0; neck(1,1) =  1; neck(1,2) = 0; neck(1,3) =   0;
    neck(2,0) = 0; neck(2,1) =  0; neck(2,2) = 1; neck(2,3) = -l1;
    neck(3,0) = 0; neck(3,1) =  0; neck(3,2) = 0; neck(3,3) =   1;
    
    if (side == 'r') {
        waist(0,0) = 0; waist(0,1) = -1; waist(0,2) = 0; waist(0,3) =   0;
        waist(1,0) = 1; waist(1,1) =  0; waist(1,2) = 0; waist(1,3) = -l2;
        waist(2,0) = 0; waist(2,1) =  0; waist(2,2) = 1; waist(2,3) = -l3;
        waist(3,0) = 0; waist(3,1) =  0; waist(3,2) = 0; waist(3,3) =   1;
        
        //        limits <<
        //        -2,   2,
        //        -2,  .2,
        //        -2,   2,
        //        -2,   0,
        //        -2,   2,
        //        -1.4, 1.2;
        
        // Set offsets
        //        offset(1) = limits(1,1);
        
    } else {
        waist(0,0) = 0; waist(0,1) = -1; waist(0,2) = 0; waist(0,3) =   0;
        waist(1,0) = 1; waist(1,1) =  0; waist(1,2) = 0; waist(1,3) =  l2;
        waist(2,0) = 0; waist(2,1) =  0; waist(2,2) = 1; waist(2,3) = -l3;
        waist(3,0) = 0; waist(3,1) =  0; waist(3,2) = 0; waist(3,3) =   1;
        
        //        limits <<
        //        -2,   2,
        //        -.3,   2,
        //        -2,   2,
        //        -2,   0,
        //        -2,   2,
        //        -1.4, 1.2;
        
        // Set offsets
        //        offset(1) = limits(1,0);
    }
    
    // Calculate forward kinematics
    B = waist*neck;
    for (int i = 0; i < 6; i++) {
        DH2HG(T, t(i)+q(i)+offset(i), f(i), r(i), d(i));
        B = B*T;
    }
}

void hubo_plus::huboLegIK(Vector6d &q, Eigen::Isometry3d B, Vector6d qPrev, int side) {
    Eigen::ArrayXXd qAll(6,8);
    
    // Declarations
    Eigen::Isometry3d neck, neckInv, waist, waistInv, BInv;
    Eigen::MatrixXd limits(6,2);
    Vector6d offset; offset.setZero();
    double nx, sx, ax, px;
    double ny, sy, ay, py;
    double nz, sz, az, pz;
    double q1, q2, q3, q4, q5, q6;
    double C45, psi, q345;
    Eigen::Matrix<int, 8, 3> m;
    
    double S2, S4, S6;
    double C2, C4, C5, C6;
    
    // Parameters
    double l1 = (79.5+107)/1000.0;
    double l2 = 88.43/1000.0;
    double l3 = (289.47-107)/1000.0;
    double l4 = 300.03/1000.0;
    double l5 = 300.38/1000.0;
    double l6 = 94.97/1000.0;
    
    neck(0,0) = 1; neck(0,1) =  0; neck(0,2) = 0; neck(0,3) =   0;
    neck(1,0) = 0; neck(1,1) =  1; neck(1,2) = 0; neck(1,3) =   0;
    neck(2,0) = 0; neck(2,1) =  0; neck(2,2) = 1; neck(2,3) = -l1;
    neck(3,0) = 0; neck(3,1) =  0; neck(3,2) = 0; neck(3,3) =   1;
    
    if (side == 'r') {
        waist(0,0) = 0; waist(0,1) = -1; waist(0,2) = 0; waist(0,3) =   0;
        waist(1,0) = 1; waist(1,1) =  0; waist(1,2) = 0; waist(1,3) = -l2;
        waist(2,0) = 0; waist(2,1) =  0; waist(2,2) = 1; waist(2,3) = -l3;
        waist(3,0) = 0; waist(3,1) =  0; waist(3,2) = 0; waist(3,3) =   1;
        
        //        limits <<
        //        -2,   2,
        //        -2,  .2,
        //        -2,   2,
        //        -2,   0,
        //        -2,   2,
        //        -1.4, 1.2;
        
        // Set offsets
        //        offset(1) = limits(1,1);
        
    } else {
        waist(0,0) = 0; waist(0,1) = -1; waist(0,2) = 0; waist(0,3) =   0;
        waist(1,0) = 1; waist(1,1) =  0; waist(1,2) = 0; waist(1,3) =  l2;
        waist(2,0) = 0; waist(2,1) =  0; waist(2,2) = 1; waist(2,3) = -l3;
        waist(3,0) = 0; waist(3,1) =  0; waist(3,2) = 0; waist(3,3) =   1;
        
        //        limits <<
        //        -2,   2,
        //        -.3,   2,
        //        -2,   2,
        //        -2,   0,
        //        -2,   2,
        //        -1.4, 1.2;
        
        // Set offsets
        //        offset(1) = limits(1,0);
    }
    neckInv = neck.inverse();
    waistInv = waist.inverse();
    
    // Variables
    B = neckInv*waistInv*B;
    BInv = B.inverse();
    
    nx = BInv(0,0); sx = BInv(0,1); ax = BInv(0,2); px = BInv(0,3);
    ny = BInv(1,0); sy = BInv(1,1); ay = BInv(1,2); py = BInv(1,3);
    nz = BInv(2,0); sz = BInv(2,1); az = BInv(2,2); pz = BInv(2,3);
    
    m <<
    1,  1,  1,
    1,  1, -1,
    1, -1,  1,
    1, -1, -1,
    -1,  1,  1,
    -1,  1, -1,
    -1, -1,  1,
    -1, -1, -1;
    
    for (int i = 0; i < 8; i++) {
        C4 = ((l6 + px)*(l6 + px) - l4*l4 - l5*l5 + py*py + pz*pz)/(2*l4*l5);
        q4 = atan2(m(i,0)*creal(sqrt(1-C4*C4)),C4);
        
        S4 = sin(q4);
        psi = atan2(S4*l4, C4*l4+l5);
        q5 = wrapToPi(atan2(-pz, m(i,1)*creal(sqrt((px+l6)*(px+l6)+py*py)))-psi);
        
        q6 = atan2(py, -px-l6);
        C45 = cos(q4+q5);
        C5 = cos(q5);
        if (C45*l4 + C5*l5 < 0) {
            q6 = wrapToPi(q6 + M_PI);
        }
        
        S6 = sin(q6);
        C6 = cos(q6);
        
        S2 = C6*ay + S6*ax;
        q2 = atan2(S2,m(i,2)*creal(sqrt(1-S2*S2)));
        
        q1 = atan2(C6*sy + S6*sx,C6*ny + S6*nx);
        C2 = cos(q2);
        if (C2 < 0) {
            q1 = wrapToPi(q1 + M_PI);
        }
        
        q345 = atan2(-az/C2,-(C6*ax - S6*ay)/C2);
        q3 = wrapToPi(q345-q4-q5);
        
        qAll(0,i) = q1;
        qAll(1,i) = q2;
        qAll(2,i) = q3;
        qAll(3,i) = q4;
        qAll(4,i) = q5;
        qAll(5,i) = q6;
    }
    
    // Set to offsets
    for (int i = 0; i < 6; i++) {
        if (side==RIGHT) {
            q(i) = wrapToPi(q(i) + offset(i));
        } else {
            q(i) = wrapToPi(q(i) + offset(i));
        }
    }
    
    // Find best solution // TODO: Need to find a better way of choosing the best solution
    Eigen::ArrayXd qDiff(6,1); qDiff.setZero();
    Eigen::ArrayXd qDiffSum(8,1);
    int minInd;
    
    for (int i = 0; i < 8; i++) {
        for (int j=0; j < 6; j++)
            qDiff(j) = wrapToPi(qAll(j,i) - qPrev(j));
        qDiffSum(i) = qDiff.abs().sum();
    }
    qDiffSum.minCoeff(&minInd);
    
    q = qAll.col(minInd);
    
    //    q = q.cwiseMin(limits.col(1));
    //    q = q.cwiseMax(limits.col(0));
}

void hubo_plus::HuboDrillFK(Eigen::Isometry3d &B, Vector6d &q) {
    Eigen::Isometry3d drill;
    
    double ld = 7*25.4/1000.0;
    double ad = M_PI/4;
    
    drill(0,0) = cos(ad);  drill(0,1) = 0; drill(0,2) = sin(ad); drill(0,3) = ld*cos(ad);
    drill(1,0) = 0;        drill(1,1) = 1; drill(1,2) = 0;       drill(1,3) = 0;
    drill(2,0) = -sin(ad); drill(2,1) = 0; drill(2,2) = cos(ad); drill(2,3) = -ld*sin(ad);
    drill(3,0) = 0;        drill(3,1) = 0; drill(3,2) = 0;       drill(3,3) = 1;
    
    HuboArmFK(B, q, 'r', drill);
}

void hubo_plus::HuboDrillIK(Vector6d &q, double y) {
    Vector6d qPrev; qPrev.setZero();
    Eigen::Isometry3d drill, B;
    
    double l1 = 214.5/1000;
    double l2 = 179.14/1000;
    double l3 = 181.59/1000;
    double l4 = 4.75*25.4/1000;
    double ld = 7*25.4/1000.0;
    double ad = M_PI/4;
    
    drill(0,0) = cos(ad);  drill(0,1) = 0; drill(0,2) = sin(ad); drill(0,3) = ld*cos(ad);
    drill(1,0) = 0;        drill(1,1) = 1; drill(1,2) = 0;       drill(1,3) = 0;
    drill(2,0) = -sin(ad); drill(2,1) = 0; drill(2,2) = cos(ad); drill(2,3) = -ld*sin(ad);
    drill(3,0) = 0;        drill(3,1) = 0; drill(3,2) = 0;       drill(3,3) = 1;
    
    double pyLimits[2] = {-0.2050, 0.1850};
    
    double px = 0.0925;
    double py = min(max(y,pyLimits[0]),pyLimits[1]);
    double pz = 0.04;
    
    double px0 = (l3+l4)*sin(M_PI/4)+ld;
    double py0 = -l1;
    double pz0 = (l3+l4)*cos(M_PI/4)-l2;
    
    B(0,0) = 1; B(0,1) = 0; B(0,2) = 0; B(0,3) = px0+px;
    B(1,0) = 0; B(1,1) = 1; B(1,2) = 0; B(1,3) = py0+py;
    B(2,0) = 0; B(2,1) = 0; B(2,2) = 1; B(2,3) = pz0+pz;
    B(3,0) = 0; B(3,1) = 0; B(3,2) = 0; B(3,3) = 1;
    
    HuboArmIK(q, B, qPrev, 'r', drill);
    
}


// ~~~*** Fastrak ***~~~ //
hp_flag_t hubo_plus::initFastrak(bool assert)
{
    int r = ach_open( &chan_fastrak, FASTRAK_CHAN_NAME, NULL );

    if( ACH_OK != r )
    {
        fprintf(stderr, "Unable to open fastrak channel: (%d) %s\n",
            r, ach_result_to_string((ach_status_t)r));
        if(assert)
            daemon_assert( ACH_OK == r, __LINE__ );
        return CHAN_OPEN_FAIL;
    }

    return SUCCESS;    
}

void hubo_plus::setFastrakScale( double scale ) { fastrakScale = scale; }
double hubo_plus::getFastrakScale() { return fastrakScale; };

hp_flag_t hubo_plus::getFastrak( Eigen::Vector3d &position, Eigen::Quaterniond &quat, int sensor, bool update )
{
    int r = ACH_OK;
    sensor--;
    if(update)
    {
        size_t fs;
        r = ach_get( &chan_fastrak, &fastrak, sizeof(fastrak), &fs, NULL, ACH_O_LAST );
        if( r == ACH_OK )
            daemon_assert( sizeof(fastrak) == fs, __LINE__ );
    }

    if( sensor < 4 )
    {
        position[0] = fastrak.data[sensor][0]/fastrakScale;
        position[1] = fastrak.data[sensor][1]/fastrakScale;
        position[2] = fastrak.data[sensor][2]/fastrakScale;

        quat.w() = (double)fastrak.data[sensor][3];
        quat.x() = (double)fastrak.data[sensor][4];
        quat.y() = (double)fastrak.data[sensor][5];
        quat.z() = (double)fastrak.data[sensor][6];
    }
    else
        return SENSOR_OOB;

    if( ACH_OK != r )
        return FASTRAK_STALE;

    return SUCCESS;
}


hp_flag_t hubo_plus::getFastrak( Eigen::Vector3d &position, Eigen::Matrix3d &rotation, int sensor, bool update )
{
    Eigen::Quaterniond quat;
    hp_flag_t flag = getFastrak( position, quat, sensor, update );

    if( flag==SENSOR_OOB )
        return flag;

    rotation = quat.matrix();

    return flag;
}


hp_flag_t hubo_plus::getFastrak( Eigen::Isometry3d &tf, int sensor, bool update )
{
    Eigen::Vector3d position;
    Eigen::Quaterniond quat;

    hp_flag_t flag = getFastrak( position, quat, sensor, update );

    if( flag==SENSOR_OOB )
        return flag;

    tf = Eigen::Matrix4d::Identity();
    tf.translate( position );
    tf.rotate( quat );

    return flag;
    
}











