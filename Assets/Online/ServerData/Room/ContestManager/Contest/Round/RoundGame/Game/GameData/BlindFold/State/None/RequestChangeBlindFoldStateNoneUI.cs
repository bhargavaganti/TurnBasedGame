﻿using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using AdvancedCoroutines;

public class RequestChangeBlindFoldStateNoneUI : UIHaveTransformDataBehavior<RequestChangeBlindFoldStateNoneUI.UIData>
{

    #region UIData

    public class UIData : RequestChangeBlindFoldUI.UIData.Sub
    {

        public VP<ReferenceData<RequestChangeBlindFoldStateNone>> requestChangeBlindFoldStateNone;

        #region state

        public enum State
        {
            None,
            Request,
            Wait
        }

        public VP<State> state;

        #endregion

        #region Constructor

        public enum Property
        {
            requestChangeBlindFoldStateNone,
            state
        }

        public UIData() : base()
        {
            this.requestChangeBlindFoldStateNone = new VP<ReferenceData<RequestChangeBlindFoldStateNone>>(this, (byte)Property.requestChangeBlindFoldStateNone, new ReferenceData<RequestChangeBlindFoldStateNone>(null));
            this.state = new VP<State>(this, (byte)Property.state, State.None);
        }

        #endregion

        public override RequestChangeBlindFold.State.Type getType()
        {
            return RequestChangeBlindFold.State.Type.None;
        }

        public override bool processEvent(Event e)
        {
            bool isProcess = false;
            {
                // shortKey
                if (!isProcess)
                {
                    if (Setting.get().useShortKey.v)
                    {
                        RequestChangeBlindFoldStateNoneUI requestChangeBlindFoldStateNoneUI = this.findCallBack<RequestChangeBlindFoldStateNoneUI>();
                        if (requestChangeBlindFoldStateNoneUI != null)
                        {
                            isProcess = requestChangeBlindFoldStateNoneUI.useShortKey(e);
                        }
                        else
                        {
                            Debug.LogError("requestChangeBlindFoldStateNoneUI null: " + this);
                        }
                    }
                }
            }
            return isProcess;
        }

        public void reset()
        {
            this.state.v = State.None;
        }

    }

    #endregion

    #region txt

    private static readonly TxtLanguage txtRequest = new TxtLanguage("Request");
    private static readonly TxtLanguage txtCancelRequest = new TxtLanguage("Cancel Request?");
    private static readonly TxtLanguage txtRequesting = new TxtLanguage("Requesting...");
    private static readonly TxtLanguage txtCannotRequest = new TxtLanguage("Cannot Request");

    private static readonly TxtLanguage txtRequestError = new TxtLanguage("send request to change rule error");

    static RequestChangeBlindFoldStateNoneUI()
    {
        txtRequest.add(Language.Type.vi, "Yêu Cầu");
        txtCancelRequest.add(Language.Type.vi, "Huỷ yêu cầu?");
        txtRequesting.add(Language.Type.vi, "Đang yêu cầu");
        txtCannotRequest.add(Language.Type.vi, "Không thể yêu cầu");

        txtRequestError.add(Language.Type.vi, "Gửi yêu cầu đổi luật lỗi");
    }

    #endregion

    #region Refresh

    public Button btnRequest;
    public Text tvRequest;

    public override void refresh()
    {
        if (dirty)
        {
            dirty = false;
            if (this.data != null)
            {
                RequestChangeBlindFoldStateNone requestChangeBlindFoldStateNone = this.data.requestChangeBlindFoldStateNone.v.data;
                if (requestChangeBlindFoldStateNone != null)
                {
                    uint profileId = Server.getProfileUserId(requestChangeBlindFoldStateNone);
                    if (requestChangeBlindFoldStateNone.isCanChange(profileId))
                    {
                        // Task
                        {
                            switch (this.data.state.v)
                            {
                                case UIData.State.None:
                                    {
                                        destroyRoutine(wait);
                                    }
                                    break;
                                case UIData.State.Request:
                                    {
                                        destroyRoutine(wait);
                                        // request
                                        {
                                            if (Server.IsServerOnline(requestChangeBlindFoldStateNone))
                                            {
                                                requestChangeBlindFoldStateNone.requestChange(profileId);
                                                this.data.state.v = UIData.State.Wait;
                                            }
                                            else
                                            {
                                                Debug.LogError("server not online: " + this);
                                            }
                                        }
                                    }
                                    break;
                                case UIData.State.Wait:
                                    {
                                        if (Server.IsServerOnline(requestChangeBlindFoldStateNone))
                                        {
                                            startRoutine(ref this.wait, TaskWait());
                                        }
                                        else
                                        {
                                            Debug.LogError("server not online: " + this);
                                            this.data.state.v = UIData.State.None;
                                            destroyRoutine(wait);
                                        }
                                    }
                                    break;
                                default:
                                    Debug.LogError("unknown state: " + this.data.state.v + "; " + this);
                                    break;
                            }
                        }
                        // UI
                        {
                            if (btnRequest != null && tvRequest != null)
                            {
                                switch (this.data.state.v)
                                {
                                    case UIData.State.None:
                                        {
                                            btnRequest.interactable = true;
                                            tvRequest.text = txtRequest.get();
                                        }
                                        break;
                                    case UIData.State.Request:
                                        {
                                            btnRequest.interactable = true;
                                            tvRequest.text = txtCancelRequest.get();
                                        }
                                        break;
                                    case UIData.State.Wait:
                                        {
                                            btnRequest.interactable = false;
                                            tvRequest.text = txtRequesting.get();
                                        }
                                        break;
                                    default:
                                        Debug.LogError("unknown state: " + this.data.state.v + "; " + this);
                                        break;
                                }
                            }
                            else
                            {
                                Debug.LogError("btnAccept, tvAccept null: " + this);
                            }
                        }
                    }
                    else
                    {
                        // Task
                        {
                            this.data.state.v = UIData.State.None;
                            destroyRoutine(wait);
                        }
                        // UI
                        {
                            if (btnRequest != null && tvRequest != null)
                            {
                                btnRequest.interactable = false;
                                tvRequest.text = txtCannotRequest.get();
                            }
                            else
                            {
                                Debug.LogError("btnRequest null, tvRequest null: " + this);
                            }
                        }
                    }
                }
                else
                {
                    // Debug.LogError("requestChangeBlindFoldStateNone null: " + this);
                }
            }
            else
            {
                // Debug.LogError("data null: " + this);
            }
        }
    }

    public override bool isShouldDisableUpdate()
    {
        return false;
    }

    #endregion

    #region Task wait

    private Routine wait;

    public IEnumerator TaskWait()
    {
        if (this.data != null)
        {
            yield return new Wait(Global.WaitSendTime);
            this.data.state.v = UIData.State.None;
            Toast.showMessage(txtRequestError.get());
            Debug.LogError("request error: " + this);
        }
        else
        {
            Debug.LogError("data null: " + this);
        }
    }

    public override List<Routine> getRoutineList()
    {
        List<Routine> ret = new List<Routine>();
        {
            ret.Add(wait);
        }
        return ret;
    }

    #endregion

    #region implement callBacks

    private RequestChangeBlindFold requestChangeBlindFold = null;
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
                uiData.requestChangeBlindFoldStateNone.allAddCallBack(this);
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
            if (data is RequestChangeBlindFoldStateNone)
            {
                RequestChangeBlindFoldStateNone requestChangeBlindFoldStateNone = data as RequestChangeBlindFoldStateNone;
                // reset
                {
                    if (this.data != null)
                    {
                        this.data.reset();
                    }
                }
                // Parent
                {
                    DataUtils.addParentCallBack(requestChangeBlindFoldStateNone, this, ref this.requestChangeBlindFold);
                    DataUtils.addParentCallBack(requestChangeBlindFoldStateNone, this, ref this.server);
                }
                dirty = true;
                return;
            }
            // Parent
            {
                // requestChangeBlindFold
                {
                    if (data is RequestChangeBlindFold)
                    {
                        RequestChangeBlindFold requestChangeBlindFold = data as RequestChangeBlindFold;
                        // Child
                        {
                            requestChangeBlindFold.whoCanAsks.allAddCallBack(this);
                        }
                        dirty = true;
                        return;
                    }
                    // Child
                    if (data is Human)
                    {
                        dirty = true;
                        return;
                    }
                }
                if (data is Server)
                {
                    dirty = true;
                    return;
                }
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
                uiData.requestChangeBlindFoldStateNone.allRemoveCallBack(this);
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
            if (data is RequestChangeBlindFoldStateNone)
            {
                RequestChangeBlindFoldStateNone requestChangeBlindFoldStateNone = data as RequestChangeBlindFoldStateNone;
                // Parent
                {
                    DataUtils.removeParentCallBack(requestChangeBlindFoldStateNone, this, ref this.requestChangeBlindFold);
                    DataUtils.removeParentCallBack(requestChangeBlindFoldStateNone, this, ref this.server);
                }
                return;
            }
            // Parent
            {
                // requestChangeBlindFold
                {
                    if (data is RequestChangeBlindFold)
                    {
                        RequestChangeBlindFold requestChangeBlindFold = data as RequestChangeBlindFold;
                        // Child
                        {
                            requestChangeBlindFold.whoCanAsks.allRemoveCallBack(this);
                        }
                        return;
                    }
                    // Child
                    if (data is Human)
                    {
                        return;
                    }
                }
                if (data is Server)
                {
                    return;
                }
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
                case UIData.Property.requestChangeBlindFoldStateNone:
                    {
                        ValueChangeUtils.replaceCallBack(this, syncs);
                        dirty = true;
                    }
                    break;
                case UIData.Property.state:
                    dirty = true;
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
            if (wrapProperty.p is RequestChangeBlindFoldStateNone)
            {
                switch ((RequestChangeBlindFoldStateNone.Property)wrapProperty.n)
                {
                    default:
                        Debug.LogError("Don't process: " + wrapProperty + "; " + this);
                        break;
                }
                return;
            }
            // Parent
            {
                // requestChangeBlindFold
                {
                    if (wrapProperty.p is RequestChangeBlindFold)
                    {
                        switch ((RequestChangeBlindFold.Property)wrapProperty.n)
                        {
                            case RequestChangeBlindFold.Property.state:
                                break;
                            case RequestChangeBlindFold.Property.whoCanAsks:
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
                    // Child
                    if (wrapProperty.p is Human)
                    {
                        Human.onUpdateSyncPlayerIdChange(wrapProperty, this);
                        return;
                    }
                }
                if (wrapProperty.p is Server)
                {
                    Server.State.OnUpdateSyncStateChange(wrapProperty, this);
                    return;
                }
            }
        }
        Debug.LogError("Don't process: " + wrapProperty + "; " + syncs + "; " + this);
    }

    #endregion

    public override void Awake()
    {
        base.Awake();
        // OnClick
        {
            UIUtils.SetButtonOnClick(btnRequest, onClickBtnRequest);
        }
    }

    public bool useShortKey(Event e)
    {
        bool isProcess = false;
        {
            if (e.isKey && e.type == EventType.KeyUp)
            {
                switch (e.keyCode)
                {
                    case KeyCode.R:
                        {
                            if (btnRequest != null && btnRequest.gameObject.activeInHierarchy && btnRequest.interactable)
                            {
                                this.onClickBtnRequest();
                                isProcess = true;
                            }
                            else
                            {
                                Debug.LogError("cannot click");
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        return isProcess;
    }

    [UnityEngine.Scripting.Preserve]
    public void onClickBtnRequest()
    {
        if (this.data != null)
        {
            switch (this.data.state.v)
            {
                case UIData.State.None:
                    this.data.state.v = UIData.State.Request;
                    break;
                case UIData.State.Request:
                    this.data.state.v = UIData.State.None;
                    break;
                case UIData.State.Wait:
                    Debug.LogError("you are requesting: " + this);
                    break;
                default:
                    Debug.LogError("unknown state: " + this.data.state.v + "; " + this);
                    break;
            }
        }
        else
        {
            Debug.LogError("data null: " + this);
        }
    }

}