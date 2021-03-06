﻿using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

namespace TimeControl.Normal
{
    public class TimeInfoUI : UIHaveTransformDataBehavior<TimeInfoUI.UIData>
    {

        #region UIData

        public class UIData : Data, EditDataUI.UIData<TimeInfo>
        {

            public VP<EditData<TimeInfo>> editTimeInfo;

            public VP<UIRectTransform.ShowType> showType;

            #region timePerTurn

            public VP<RequestChangeEnumUI.UIData> timePerTurnType;

            public void makeRequestChangeTimePerTurnType(RequestChangeUpdate<int>.UpdateData update, int newTimePerTurnType)
            {
                // Find
                TimeInfo timeInfo = null;
                {
                    EditData<TimeInfo> editTimeInfo = this.editTimeInfo.v;
                    if (editTimeInfo != null)
                    {
                        timeInfo = editTimeInfo.show.v.data;
                    }
                    else
                    {
                        Debug.LogError("editTimeInfo null: " + this);
                    }
                }
                // Process
                if (timeInfo != null)
                {
                    timeInfo.requestChangeTimePerTurnType(Server.getProfileUserId(timeInfo), newTimePerTurnType);
                }
                else
                {
                    Debug.LogError("timeInfo null: " + this);
                }
            }

            public VP<TimePerTurnInfoUI.UIData> timePerTurn;

            #endregion

            #region totalTime

            public VP<RequestChangeEnumUI.UIData> totalTimeType;

            public void makeRequestChangeTotalTimeType(RequestChangeUpdate<int>.UpdateData update, int newTotalTimeType)
            {
                // Find
                TimeInfo timeInfo = null;
                {
                    EditData<TimeInfo> editTimeInfo = this.editTimeInfo.v;
                    if (editTimeInfo != null)
                    {
                        timeInfo = editTimeInfo.show.v.data;
                    }
                    else
                    {
                        Debug.LogError("editTimeInfo null: " + this);
                    }
                }
                // Process
                if (timeInfo != null)
                {
                    timeInfo.requestChangeTotalTimeType(Server.getProfileUserId(timeInfo), newTotalTimeType);
                }
                else
                {
                    Debug.LogError("timeInfo null: " + this);
                }
            }

            public VP<TotalTimeInfoUI.UIData> totalTime;

            #endregion

            #region overTimePerTurn

            public VP<RequestChangeEnumUI.UIData> overTimePerTurnType;

            public void makeRequestChangeOverTimePerTurnType(RequestChangeUpdate<int>.UpdateData update, int newOverTimePerTurnType)
            {
                // Find
                TimeInfo timeInfo = null;
                {
                    EditData<TimeInfo> editTimeInfo = this.editTimeInfo.v;
                    if (editTimeInfo != null)
                    {
                        timeInfo = editTimeInfo.show.v.data;
                    }
                    else
                    {
                        Debug.LogError("editTimeInfo null: " + this);
                    }
                }
                // Process
                if (timeInfo != null)
                {
                    timeInfo.requestChangeOverTimePerTurnType(Server.getProfileUserId(timeInfo), newOverTimePerTurnType);
                }
                else
                {
                    Debug.LogError("timeInfo null: " + this);
                }
            }

            public VP<TimePerTurnInfoUI.UIData> overTimePerTurn;

            #endregion

            #region lagCompensation

            public VP<RequestChangeFloatUI.UIData> lagCompensation;

            public void makeRequestChangeLagCompensation(RequestChangeUpdate<float>.UpdateData update, float newLagCompensation)
            {
                // Find
                TimeInfo timeInfo = null;
                {
                    EditData<TimeInfo> editTimeInfo = this.editTimeInfo.v;
                    if (editTimeInfo != null)
                    {
                        timeInfo = editTimeInfo.show.v.data;
                    }
                    else
                    {
                        Debug.LogError("editTimeInfo null: " + this);
                    }
                }
                // Process
                if (timeInfo != null)
                {
                    timeInfo.requestChangeLagCompensation(Server.getProfileUserId(timeInfo), newLagCompensation);
                }
                else
                {
                    Debug.LogError("timeInfo null: " + this);
                }
            }

            #endregion

            #region Constructor

            public enum Property
            {
                editTimeInfo,
                showType,
                timePerTurnType,
                timePerTurn,
                totalTimeType,
                totalTime,
                overTimePerTurnType,
                overTimePerTurn,
                lagCompensation
            }

            public UIData() : base()
            {
                this.editTimeInfo = new VP<EditData<TimeInfo>>(this, (byte)Property.editTimeInfo, new EditData<TimeInfo>());
                this.showType = new VP<UIRectTransform.ShowType>(this, (byte)Property.showType, UIRectTransform.ShowType.Normal);
                // timePerTurnType
                {
                    this.timePerTurnType = new VP<RequestChangeEnumUI.UIData>(this, (byte)Property.timePerTurnType, new RequestChangeEnumUI.UIData());
                    // event
                    this.timePerTurnType.v.updateData.v.request.v = makeRequestChangeTimePerTurnType;
                    {
                        foreach (TimePerTurnInfo.Type type in System.Enum.GetValues(typeof(TimePerTurnInfo.Type)))
                        {
                            this.timePerTurnType.v.options.add(type.ToString());
                        }
                    }
                }
                this.timePerTurn = new VP<TimePerTurnInfoUI.UIData>(this, (byte)Property.timePerTurn, new TimePerTurnInfoUI.UIData());
                // totalTimeType
                {
                    this.totalTimeType = new VP<RequestChangeEnumUI.UIData>(this, (byte)Property.totalTimeType, new RequestChangeEnumUI.UIData());
                    // event
                    this.totalTimeType.v.updateData.v.request.v = makeRequestChangeTotalTimeType;
                    {
                        foreach (TotalTimeInfo.Type type in System.Enum.GetValues(typeof(TotalTimeInfo.Type)))
                        {
                            this.totalTimeType.v.options.add(type.ToString());
                        }
                    }
                }
                this.totalTime = new VP<TotalTimeInfoUI.UIData>(this, (byte)Property.totalTime, new TotalTimeInfoUI.UIData());
                // overTimePerTurnType
                {
                    this.overTimePerTurnType = new VP<RequestChangeEnumUI.UIData>(this, (byte)Property.overTimePerTurnType, new RequestChangeEnumUI.UIData());
                    // event
                    this.overTimePerTurnType.v.updateData.v.request.v = makeRequestChangeOverTimePerTurnType;
                    {
                        foreach (TimePerTurnInfo.Type type in System.Enum.GetValues(typeof(TimePerTurnInfo.Type)))
                        {
                            this.overTimePerTurnType.v.options.add(type.ToString());
                        }
                    }
                }
                this.overTimePerTurn = new VP<TimePerTurnInfoUI.UIData>(this, (byte)Property.overTimePerTurn, new TimePerTurnInfoUI.UIData());
                // lagCompensation
                {
                    this.lagCompensation = new VP<RequestChangeFloatUI.UIData>(this, (byte)Property.lagCompensation, new RequestChangeFloatUI.UIData());
                    // event
                    this.lagCompensation.v.updateData.v.request.v = makeRequestChangeLagCompensation;
                }
            }

            #endregion

            #region implement interface

            public EditData<TimeInfo> getEditData()
            {
                return this.editTimeInfo.v;
            }

            #endregion

        }

        #endregion

        #region txt

        public Text lbTitle;
        private static readonly TxtLanguage txtTitle = new TxtLanguage("Time Information");

        public Text lbTimePerTurnType;
        private static readonly TxtLanguage txtTimePerTurnType = new TxtLanguage("Time per turn type");

        public Text lbTotalTimeType;
        private static readonly TxtLanguage txtTotalTimeType = new TxtLanguage("Total time type");

        public Text lbOverTimePerTurnType;
        private static readonly TxtLanguage txtOverTimePerTurnType = new TxtLanguage("Over time per turn type");

        public Text lbLagCompensation;
        private static readonly TxtLanguage txtLagCompensation = new TxtLanguage("Lag compensation");

        static TimeInfoUI()
        {
            // txt
            {
                txtTitle.add(Language.Type.vi, "Thông Tin Thời Gian");
                txtTimePerTurnType.add(Language.Type.vi, "Loại thời gian mỗi lượt");
                txtTotalTimeType.add(Language.Type.vi, "Loại tổng thời gian");
                txtOverTimePerTurnType.add(Language.Type.vi, "Loại quá thời gian mỗi lượt");
                txtLagCompensation.add(Language.Type.vi, "Đền bù lag");
            }
            // rect
            {

            }
        }

        #endregion

        #region Refresh

        private bool needReset = true;

        public Image bgTimePerTurn;
        public Image bgTotalTime;
        public Image bgOverTimePerTurn;

        public override void refresh()
        {
            if (dirty)
            {
                dirty = false;
                if (this.data != null)
                {
                    EditData<TimeInfo> editTimeInfo = this.data.editTimeInfo.v;
                    if (editTimeInfo != null)
                    {
                        // update
                        editTimeInfo.update();
                        // UI
                        {
                            // different
                            RequestChange.ShowDifferentTitle(lbTitle, editTimeInfo);
                            // request
                            {
                                // get server state
                                Server.State.Type serverState = RequestChange.GetServerState(editTimeInfo);
                                // set origin
                                {
                                    // timePerTurnType
                                    {
                                        RequestChangeEnumUI.RefreshOptions(this.data.timePerTurnType.v, TimePerTurnInfo.getStrTypes());
                                        RequestChange.RefreshUI(this.data.timePerTurnType.v, editTimeInfo, serverState, needReset, editData => (int)editData.getTimePerTurnType());
                                    }
                                    // timePerTurn
                                    {
                                        EditDataUI.RefreshChildUI(this.data, this.data.timePerTurn.v, editData => editData.timePerTurn.v);
                                        // showType
                                        {
                                            if (this.data.timePerTurn.v != null)
                                            {
                                                this.data.timePerTurn.v.showType.v = UIRectTransform.ShowType.HeadLess;
                                            }
                                            else
                                            {
                                                Debug.LogError("timePerTurn null");
                                            }
                                        }
                                    }
                                    // totalTimeType
                                    {
                                        RequestChangeEnumUI.RefreshOptions(this.data.totalTimeType.v, TotalTimeInfo.getStrTypes());
                                        RequestChange.RefreshUI(this.data.totalTimeType.v, editTimeInfo, serverState, needReset, editData => (int)editData.getTotalTimeType());
                                    }
                                    // totalTime
                                    {
                                        EditDataUI.RefreshChildUI(this.data, this.data.totalTime.v, editData => editData.totalTime.v);
                                        // showType
                                        {
                                            TotalTimeInfoUI.UIData totalTime = this.data.totalTime.v;
                                            if (totalTime != null)
                                            {
                                                totalTime.showType.v = UIRectTransform.ShowType.HeadLess;
                                            }
                                            else
                                            {
                                                Debug.LogError("totalTime null");
                                            }
                                        }
                                    }
                                    // overTimePerTurnType
                                    {
                                        RequestChangeEnumUI.RefreshOptions(this.data.overTimePerTurnType.v, TimePerTurnInfo.getStrTypes());
                                        RequestChange.RefreshUI(this.data.overTimePerTurnType.v, editTimeInfo, serverState, needReset, editData => (int)editData.getOverTimePerTurnType());
                                    }
                                    // overTimePerTurn
                                    {
                                        EditDataUI.RefreshChildUI(this.data, this.data.overTimePerTurn.v, editData => editData.overTimePerTurn.v);
                                        // showType
                                        {
                                            TimePerTurnInfoUI.UIData overTimePerTurn = this.data.overTimePerTurn.v;
                                            if (overTimePerTurn != null)
                                            {
                                                overTimePerTurn.showType.v = UIRectTransform.ShowType.HeadLess;
                                            }
                                            else
                                            {
                                                Debug.LogError("overTimePerTurn null");
                                            }
                                        }
                                    }
                                    RequestChange.RefreshUI(this.data.lagCompensation.v, editTimeInfo, serverState, needReset, editData => editData.lagCompensation.v);
                                }
                                needReset = false;
                            }
                        }
                        // UI Position
                        {
                            float deltaY = 0;
                            // header
                            UIUtils.SetHeaderPosition(lbTitle, this.data.showType.v, ref deltaY);
                            // timePerTurn
                            {
                                float bgY = deltaY;
                                float bgHeight = 0;
                                // type
                                UIUtils.SetLabelContentPositionBg(lbTimePerTurnType, this.data.timePerTurnType.v, ref deltaY, ref bgHeight);
                                // UI
                                {
                                    float height = UIRectTransform.SetPosY(this.data.timePerTurn.v, deltaY);
                                    bgHeight += height;
                                    deltaY += height;
                                }
                                // bg
                                if (bgTimePerTurn != null)
                                {
                                    UIRectTransform.SetPosY(bgTimePerTurn.rectTransform, bgY);
                                    UIRectTransform.SetHeight(bgTimePerTurn.rectTransform, bgHeight);
                                }
                                else
                                {
                                    Debug.LogError("bgTimePerTurn null");
                                }
                            }
                            // totalTime
                            {
                                float bgY = deltaY;
                                float bgHeight = 0;
                                // type
                                UIUtils.SetLabelContentPositionBg(lbTotalTimeType, this.data.totalTimeType.v, ref deltaY, ref bgHeight);
                                // UI
                                {
                                    float height = UIRectTransform.SetPosY(this.data.totalTime.v, deltaY);
                                    bgHeight += height;
                                    deltaY += height;
                                }
                                // bg
                                if (bgTotalTime != null)
                                {
                                    UIRectTransform.SetPosY(bgTotalTime.rectTransform, bgY);
                                    UIRectTransform.SetHeight(bgTotalTime.rectTransform, bgHeight);
                                }
                                else
                                {
                                    Debug.LogError("bgTotalTime null");
                                }
                            }
                            // overTimePerTurn
                            {
                                float bgY = deltaY;
                                float bgHeight = 0;
                                // Type
                                UIUtils.SetLabelContentPositionBg(lbOverTimePerTurnType, this.data.overTimePerTurnType.v, ref deltaY, ref bgHeight);
                                // UI
                                {
                                    float height = UIRectTransform.SetPosY(this.data.overTimePerTurn.v, deltaY);
                                    bgHeight += height;
                                    deltaY += height;
                                }
                                // bg
                                if (bgOverTimePerTurn != null)
                                {
                                    UIRectTransform.SetPosY(bgOverTimePerTurn.rectTransform, bgY);
                                    UIRectTransform.SetHeight(bgOverTimePerTurn.rectTransform, bgHeight);
                                }
                                else
                                {
                                    Debug.LogError("bgOverTimePerTurn null");
                                }
                            }
                            // lagCompensation
                            UIUtils.SetLabelContentPosition(lbLagCompensation, this.data.lagCompensation.v, ref deltaY);
                            // set
                            UIRectTransform.SetHeight((RectTransform)this.transform, deltaY);
                        }
                        // txt
                        {
                            if (lbTitle != null)
                            {
                                lbTitle.text = txtTitle.get();
                                Setting.get().setTitleTextSize(lbTitle);
                            }
                            else
                            {
                                Debug.LogError("lbTitle null: " + this);
                            }
                            if (lbTimePerTurnType != null)
                            {
                                lbTimePerTurnType.text = txtTimePerTurnType.get();
                                Setting.get().setLabelTextSize(lbTimePerTurnType);
                            }
                            else
                            {
                                Debug.LogError("lbTimePerTurnType null: " + this);
                            }
                            if (lbTotalTimeType != null)
                            {
                                lbTotalTimeType.text = txtTotalTimeType.get();
                                Setting.get().setLabelTextSize(lbTotalTimeType);
                            }
                            else
                            {
                                Debug.LogError("lbTotalTimeType null: " + this);
                            }
                            if (lbOverTimePerTurnType != null)
                            {
                                lbOverTimePerTurnType.text = txtOverTimePerTurnType.get();
                                Setting.get().setLabelTextSize(lbOverTimePerTurnType);
                            }
                            else
                            {
                                Debug.LogError("lbOverTimePerTurnType null: " + this);
                            }
                            if (lbLagCompensation != null)
                            {
                                lbLagCompensation.text = txtLagCompensation.get();
                                Setting.get().setLabelTextSize(lbLagCompensation);
                            }
                            else
                            {
                                Debug.LogError("lbLagCompensation null: " + this);
                            }
                        }
                    }
                    else
                    {
                        Debug.LogError("editTimeInfo null: " + this);
                    }
                }
                else
                {
                    Debug.LogError("data null: " + this);
                }
            }
        }

        public override bool isShouldDisableUpdate()
        {
            return true;
        }

        #endregion

        #region implement callBacks

        public TimePerTurnInfoUI timePerTurnPrefab;
        public TotalTimeInfoUI totalTimePrefab;

        private Server server = null;

        public override void onAddCallBack<T>(T data)
        {
            if (data is UIData)
            {
                UIData uiData = data as UIData;
                // Setting
                Setting.get().addCallBack(this);
                // Child
                {
                    uiData.editTimeInfo.allAddCallBack(this);
                    uiData.timePerTurnType.allAddCallBack(this);
                    uiData.timePerTurn.allAddCallBack(this);
                    uiData.totalTimeType.allAddCallBack(this);
                    uiData.totalTime.allAddCallBack(this);
                    uiData.overTimePerTurnType.allAddCallBack(this);
                    uiData.overTimePerTurn.allAddCallBack(this);
                    uiData.lagCompensation.allAddCallBack(this);
                }
                dirty = true;
                return;
            }
            // Setting
            if (data is Setting)
            {
                dirty = true;
                return;
            }
            // Child
            {
                // editTimeInfo
                {
                    if (data is EditData<TimeInfo>)
                    {
                        EditData<TimeInfo> editTimeInfo = data as EditData<TimeInfo>;
                        // Child
                        {
                            editTimeInfo.show.allAddCallBack(this);
                            editTimeInfo.compare.allAddCallBack(this);
                        }
                        dirty = true;
                        return;
                    }
                    // Child
                    {
                        if (data is TimeInfo)
                        {
                            TimeInfo timeInfo = data as TimeInfo;
                            // Parent
                            {
                                DataUtils.addParentCallBack(timeInfo, this, ref this.server);
                            }
                            needReset = true;
                            dirty = true;
                            return;
                        }
                        // Parent
                        {
                            if (data is Server)
                            {
                                dirty = true;
                                return;
                            }
                        }
                    }
                }
                if (data is RequestChangeEnumUI.UIData)
                {
                    RequestChangeEnumUI.UIData requestChange = data as RequestChangeEnumUI.UIData;
                    // UI
                    {
                        WrapProperty wrapProperty = requestChange.p;
                        if (wrapProperty != null)
                        {
                            switch ((UIData.Property)wrapProperty.n)
                            {
                                case UIData.Property.timePerTurnType:
                                    UIUtils.Instantiate(requestChange, GlobalPrefab.instance.requestEnum, this.transform, UIConstants.RequestEnumRect);
                                    break;
                                case UIData.Property.totalTimeType:
                                    UIUtils.Instantiate(requestChange, GlobalPrefab.instance.requestEnum, this.transform, UIConstants.RequestEnumRect);
                                    break;
                                case UIData.Property.overTimePerTurnType:
                                    UIUtils.Instantiate(requestChange, GlobalPrefab.instance.requestEnum, this.transform, UIConstants.RequestEnumRect);
                                    break;
                                default:
                                    Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                                    break;
                            }
                        }
                        else
                        {
                            Debug.LogError("wrapProperty null: " + this);
                        }
                    }
                    dirty = true;
                    return;
                }
                // InfoUI
                {
                    if (data is TimePerTurnInfoUI.UIData)
                    {
                        TimePerTurnInfoUI.UIData timePerTurnInfoUIData = data as TimePerTurnInfoUI.UIData;
                        // UI
                        {
                            WrapProperty wrapProperty = timePerTurnInfoUIData.p;
                            if (wrapProperty != null)
                            {
                                switch ((UIData.Property)wrapProperty.n)
                                {
                                    case UIData.Property.timePerTurn:
                                        UIUtils.Instantiate(timePerTurnInfoUIData, timePerTurnPrefab, this.transform);
                                        break;
                                    case UIData.Property.overTimePerTurn:
                                        UIUtils.Instantiate(timePerTurnInfoUIData, timePerTurnPrefab, this.transform);
                                        break;
                                    default:
                                        Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                                        break;
                                }
                            }
                            else
                            {
                                Debug.LogError("wrapProperty null: " + this);
                            }
                        }
                        // Child
                        {
                            TransformData.AddCallBack(timePerTurnInfoUIData, this);
                        }
                        dirty = true;
                        return;
                    }
                    if (data is TotalTimeInfoUI.UIData)
                    {
                        TotalTimeInfoUI.UIData totalTimeInfoUIData = data as TotalTimeInfoUI.UIData;
                        // UI
                        {
                            UIUtils.Instantiate(totalTimeInfoUIData, totalTimePrefab, this.transform);
                        }
                        // Child
                        {
                            TransformData.AddCallBack(totalTimeInfoUIData, this);
                        }
                        dirty = true;
                        return;
                    }
                    // Child
                    if (data is TransformData)
                    {
                        dirty = true;
                        return;
                    }
                }
                if (data is RequestChangeFloatUI.UIData)
                {
                    RequestChangeFloatUI.UIData requestChange = data as RequestChangeFloatUI.UIData;
                    // UI
                    {
                        WrapProperty wrapProperty = requestChange.p;
                        if (wrapProperty != null)
                        {
                            switch ((UIData.Property)wrapProperty.n)
                            {
                                case UIData.Property.lagCompensation:
                                    UIUtils.Instantiate(requestChange, GlobalPrefab.instance.requestFloat, this.transform, UIConstants.RequestRect);
                                    break;
                                default:
                                    Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                                    break;
                            }
                        }
                        else
                        {
                            Debug.LogError("wrapProperty null: " + this);
                        }
                    }
                    dirty = true;
                    return;
                }
            }
            Debug.LogError("Don't process: " + data + "; " + this);
        }

        public override void onRemoveCallBack<T>(T data, bool isHide)
        {
            if (data is UIData)
            {
                UIData uiData = data as UIData;
                // Setting
                Setting.get().removeCallBack(this);
                // Child
                {
                    uiData.editTimeInfo.allRemoveCallBack(this);
                    uiData.timePerTurnType.allRemoveCallBack(this);
                    uiData.timePerTurn.allRemoveCallBack(this);
                    uiData.totalTimeType.allRemoveCallBack(this);
                    uiData.totalTime.allRemoveCallBack(this);
                    uiData.overTimePerTurnType.allRemoveCallBack(this);
                    uiData.overTimePerTurn.allRemoveCallBack(this);
                    uiData.lagCompensation.allRemoveCallBack(this);
                }
                this.setDataNull(uiData);
                return;
            }
            // Setting
            if (data is Setting)
            {
                return;
            }
            // Child
            {
                // editTimeInfo
                {
                    if (data is EditData<TimeInfo>)
                    {
                        EditData<TimeInfo> editTimeInfo = data as EditData<TimeInfo>;
                        // Child
                        {
                            editTimeInfo.show.allRemoveCallBack(this);
                            editTimeInfo.compare.allRemoveCallBack(this);
                        }
                        return;
                    }
                    // Child
                    {
                        if (data is TimeInfo)
                        {
                            TimeInfo timeInfo = data as TimeInfo;
                            // Parent
                            {
                                DataUtils.removeParentCallBack(timeInfo, this, ref this.server);
                            }
                            return;
                        }
                        // Parent
                        {
                            if (data is Server)
                            {
                                return;
                            }
                        }
                    }
                }
                if (data is RequestChangeEnumUI.UIData)
                {
                    RequestChangeEnumUI.UIData requestChange = data as RequestChangeEnumUI.UIData;
                    // UI
                    {
                        requestChange.removeCallBackAndDestroy(typeof(RequestChangeEnumUI));
                    }
                    return;
                }
                // InfoUI
                {
                    if (data is TimePerTurnInfoUI.UIData)
                    {
                        TimePerTurnInfoUI.UIData timePerTurnInfoUIData = data as TimePerTurnInfoUI.UIData;
                        // Child
                        {
                            TransformData.RemoveCallBack(timePerTurnInfoUIData, this);
                        }
                        // UI
                        {
                            timePerTurnInfoUIData.removeCallBackAndDestroy(typeof(TimePerTurnInfoUI));
                        }
                        return;
                    }
                    if (data is TotalTimeInfoUI.UIData)
                    {
                        TotalTimeInfoUI.UIData totalTimeInfoUIData = data as TotalTimeInfoUI.UIData;
                        // Child
                        {
                            TransformData.RemoveCallBack(totalTimeInfoUIData, this);
                        }
                        // UI
                        {
                            totalTimeInfoUIData.removeCallBackAndDestroy(typeof(TotalTimeInfoUI));
                        }
                        return;
                    }
                    // Child
                    if (data is TransformData)
                    {
                        dirty = true;
                        return;
                    }
                }
                if (data is RequestChangeFloatUI.UIData)
                {
                    RequestChangeFloatUI.UIData requestChange = data as RequestChangeFloatUI.UIData;
                    // UI
                    {
                        requestChange.removeCallBackAndDestroy(typeof(RequestChangeFloatUI));
                    }
                    return;
                }
            }
            Debug.LogError("Don't process: " + data + "; " + this);
        }

        public override void onUpdateSync<T>(WrapProperty wrapProperty, List<Sync<T>> syncs)
        {
            if (WrapProperty.checkError(wrapProperty))
            {
                return;
            }
            if (wrapProperty.p is UIData)
            {
                switch ((UIData.Property)wrapProperty.n)
                {
                    case UIData.Property.editTimeInfo:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.showType:
                        dirty = true;
                        break;
                    case UIData.Property.timePerTurnType:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.timePerTurn:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.totalTimeType:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.totalTime:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.overTimePerTurnType:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.overTimePerTurn:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    case UIData.Property.lagCompensation:
                        {
                            ValueChangeUtils.replaceCallBack(this, syncs);
                            dirty = true;
                        }
                        break;
                    default:
                        Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                        break;
                }
                return;
            }
            // Setting
            if (wrapProperty.p is Setting)
            {
                switch ((Setting.Property)wrapProperty.n)
                {
                    case Setting.Property.language:
                        dirty = true;
                        break;
                    case Setting.Property.style:
                        break;
                    case Setting.Property.contentTextSize:
                        dirty = true;
                        break;
                    case Setting.Property.titleTextSize:
                        dirty = true;
                        break;
                    case Setting.Property.labelTextSize:
                        dirty = true;
                        break;
                    case Setting.Property.buttonSize:
                        dirty = true;
                        break;
                    case Setting.Property.itemSize:
                        dirty = true;
                        break;
                    case Setting.Property.confirmQuit:
                        break;
                    case Setting.Property.showLastMove:
                        break;
                    case Setting.Property.viewUrlImage:
                        break;
                    case Setting.Property.animationSetting:
                        break;
                    case Setting.Property.maxThinkCount:
                        break;
                    default:
                        Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                        break;
                }
                return;
            }
            // Child
            {
                // editTimeInfo
                {
                    if (wrapProperty.p is EditData<TimeInfo>)
                    {
                        switch ((EditData<TimeInfo>.Property)wrapProperty.n)
                        {
                            case EditData<TimeInfo>.Property.origin:
                                dirty = true;
                                break;
                            case EditData<TimeInfo>.Property.show:
                                {
                                    ValueChangeUtils.replaceCallBack(this, syncs);
                                    dirty = true;
                                }
                                break;
                            case EditData<TimeInfo>.Property.compare:
                                {
                                    ValueChangeUtils.replaceCallBack(this, syncs);
                                    dirty = true;
                                }
                                break;
                            case EditData<TimeInfo>.Property.compareOtherType:
                                dirty = true;
                                break;
                            case EditData<TimeInfo>.Property.canEdit:
                                dirty = true;
                                break;
                            case EditData<TimeInfo>.Property.editType:
                                dirty = true;
                                break;
                            default:
                                Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                                break;
                        }
                        return;
                    }
                    // Child
                    {
                        if (wrapProperty.p is TimeInfo)
                        {
                            switch ((TimeInfo.Property)wrapProperty.n)
                            {
                                case TimeInfo.Property.timePerTurn:
                                    dirty = true;
                                    break;
                                case TimeInfo.Property.totalTime:
                                    dirty = true;
                                    break;
                                case TimeInfo.Property.overTimePerTurn:
                                    dirty = true;
                                    break;
                                case TimeInfo.Property.lagCompensation:
                                    dirty = true;
                                    break;
                                default:
                                    Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                                    break;
                            }
                            return;
                        }
                        // Parent
                        {
                            if (wrapProperty.p is Server)
                            {
                                Server.State.OnUpdateSyncStateChange(wrapProperty, this);
                                return;
                            }
                        }
                    }
                }
                if (wrapProperty.p is RequestChangeEnumUI.UIData)
                {
                    return;
                }
                // InfoUI
                {
                    if (wrapProperty.p is TimePerTurnInfoUI.UIData)
                    {
                        return;
                    }
                    if (wrapProperty.p is TotalTimeInfoUI.UIData)
                    {
                        return;
                    }
                    // Child
                    if (wrapProperty.p is TransformData)
                    {
                        switch ((TransformData.Property)wrapProperty.n)
                        {
                            case TransformData.Property.anchoredPosition:
                                break;
                            case TransformData.Property.anchorMin:
                                break;
                            case TransformData.Property.anchorMax:
                                break;
                            case TransformData.Property.pivot:
                                break;
                            case TransformData.Property.offsetMin:
                                break;
                            case TransformData.Property.offsetMax:
                                break;
                            case TransformData.Property.sizeDelta:
                                break;
                            case TransformData.Property.rotation:
                                break;
                            case TransformData.Property.scale:
                                break;
                            case TransformData.Property.size:
                                dirty = true;
                                break;
                            default:
                                Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                                break;
                        }
                        return;
                    }
                }
                if (wrapProperty.p is RequestChangeFloatUI.UIData)
                {
                    return;
                }
            }
            Debug.LogError("Don't process: " + wrapProperty + "; " + syncs + "; " + this);
        }

        #endregion

    }
}