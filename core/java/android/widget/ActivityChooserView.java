/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.widget;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.TypedArray;
import android.database.DataSetObserver;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.Debug;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ActivityChooserModel.ActivityChooserModelClient;

import com.android.internal.R;

/**
 * This class is a view for choosing an activity for handling a given {@link Intent}.
 * <p>
 * The view is composed of two adjacent buttons:
 * <ul>
 * <li>
 * The left button is an immediate action and allows one click activity choosing.
 * Tapping this button immediately executes the intent without requiring any further
 * user input. Long press on this button shows a popup for changing the default
 * activity.
 * </li>
 * <li>
 * The right button is an overflow action and provides an optimized menu
 * of additional activities. Tapping this button shows a popup anchored to this
 * view, listing the most frequently used activities. This list is initially
 * limited to a small number of items in frequency used order. The last item,
 * "Show all..." serves as an affordance to display all available activities.
 * </li>
 * </ul>
 * </p>
 * </p>
 * This view is backed by a {@link ActivityChooserModel}. Calling {@link #showPopup()}
 * while this view is attached to the view hierarchy will show a popup with
 * activities while if the view is not attached it will show a dialog.
 * </p>
 *
 * @hide
 */
public class ActivityChooserView extends ViewGroup implements ActivityChooserModelClient {

    /**
     * An adapter for displaying the activities in an {@link AdapterView}.
     */
    private final ActivityChooserViewAdapter mAdapter;

    /**
     * Implementation of various interfaces to avoid publishing them in the APIs.
     */
    private final Callbacks mCallbacks;

    /**
     * The content of this view.
     */
    private final LinearLayout mActivityChooserContent;

    /**
     * The expand activities action button;
     */
    private final ImageButton mExpandActivityOverflowButton;

    /**
     * The default activities action button;
     */
    private final ImageButton mDefaultActionButton;

    /**
     * The header for handlers list.
     */
    private final View mListHeaderView;

    /**
     * The footer for handlers list.
     */
    private final View mListFooterView;

    /**
     * The title of the header view.
     */
    private TextView mListHeaderViewTitle;

    /**
     * The title for expanding the activities list.
     */
    private final String mListHeaderViewTitleSelectDefault;

    /**
     * The title if no activity exist.
     */
    private final String mListHeaderViewTitleNoActivities;

    /**
     * Popup window for showing the activity overflow list.
     */
    private ListPopupWindow mListPopupWindow;

    /**
     * Alert dialog for showing the activity overflow list.
     */
    private AlertDialog mAlertDialog;

    /**
     * Listener for the dismissal of the popup/alert.
     */
    private PopupWindow.OnDismissListener mOnDismissListener;

    /**
     * Flag whether a default activity currently being selected.
     */
    private boolean mIsSelectingDefaultActivity;

    /**
     * The count of activities in the popup.
     */
    private int mInitialActivityCount = ActivityChooserViewAdapter.MAX_ACTIVITY_COUNT_DEFAULT;

    /**
     * Flag whether this view is attached to a window.
     */
    private boolean mIsAttachedToWindow;

    /**
     * Flag whether this view is showing an alert dialog.
     */
    private boolean mIsShowingAlertDialog;

    /**
     * Flag whether this view is showing a popup window.
     */
    private boolean mIsShowingPopuWindow;

    /**
     * Create a new instance.
     *
     * @param context The application environment.
     */
    public ActivityChooserView(Context context) {
        this(context, null);
    }

    /**
     * Create a new instance.
     *
     * @param context The application environment.
     * @param attrs A collection of attributes.
     */
    public ActivityChooserView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.actionButtonStyle);
    }

    /**
     * Create a new instance.
     *
     * @param context The application environment.
     * @param attrs A collection of attributes.
     * @param defStyle The default style to apply to this view.
     */
    public ActivityChooserView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        TypedArray attributesArray = context.obtainStyledAttributes(attrs,
                R.styleable.ActivityChooserView, defStyle, 0);

        mInitialActivityCount = attributesArray.getInt(
                R.styleable.ActivityChooserView_initialActivityCount,
                ActivityChooserViewAdapter.MAX_ACTIVITY_COUNT_DEFAULT);

        Drawable expandActivityOverflowButtonDrawable = attributesArray.getDrawable(
                R.styleable.ActivityChooserView_expandActivityOverflowButtonDrawable);

        LayoutInflater inflater = (LayoutInflater) context.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.activity_chooser_view, this, true);

        mCallbacks = new Callbacks();

        mActivityChooserContent = (LinearLayout) findViewById(R.id.activity_chooser_view_content);

        mDefaultActionButton = (ImageButton) findViewById(R.id.default_activity_button);
        mDefaultActionButton.setOnClickListener(mCallbacks);
        mDefaultActionButton.setOnLongClickListener(mCallbacks);

        mExpandActivityOverflowButton = (ImageButton) findViewById(R.id.expand_activities_button);
        mExpandActivityOverflowButton.setOnClickListener(mCallbacks);
        mExpandActivityOverflowButton.setBackgroundDrawable(expandActivityOverflowButtonDrawable);

        mListHeaderView = inflater.inflate(R.layout.activity_chooser_list_header, null);
        mListFooterView = inflater.inflate(R.layout.activity_chooser_list_footer, null);

        mListHeaderViewTitle = (TextView) mListHeaderView.findViewById(R.id.title);
        mListHeaderViewTitleSelectDefault = context.getString(
                R.string.activity_chooser_view_select_default);
        mListHeaderViewTitleNoActivities = context.getString(
                R.string.activity_chooser_view_no_activities);

        mAdapter = new ActivityChooserViewAdapter();
        mAdapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                super.onChanged();
                updateButtons();
            }
        });
    }

    /**
     * {@inheritDoc}
     */
    public void setActivityChooserModel(ActivityChooserModel dataModel) {
        mAdapter.setDataModel(dataModel);
        if (isShowingPopup()) {
            dismissPopup();
            showPopup();
        }
    }

    /**
     * Sets the background for the button that expands the activity
     * overflow list.
     *
     * <strong>Note:</strong> Clients would like to set this drawable
     * as a clue about the action the chosen activity will perform. For
     * example, if share activity is to be chosen the drawable should
     * give a clue that sharing is to be performed.
     *
     * @param drawable The drawable.
     */
    public void setExpandActivityOverflowButtonDrawable(Drawable drawable) {
        mExpandActivityOverflowButton.setBackgroundDrawable(drawable);
    }

    /**
     * Shows the popup window with activities.
     *
     * @return True if the popup was shown, false if already showing.
     */
    public boolean showPopup() {
        if (isShowingPopup()) {
            return false;
        }
        mIsSelectingDefaultActivity = false;
        showPopupUnchecked(mInitialActivityCount);
        return true;
    }

    /**
     * Shows the popup no matter if it was already showing.
     *
     * @param maxActivityCount The max number of activities to display.
     */
    private void showPopupUnchecked(int maxActivityCount) {
        mAdapter.setMaxActivityCount(maxActivityCount);
        if (mIsSelectingDefaultActivity) {
            if (mAdapter.getActivityCount() > 0) {
                mListHeaderViewTitle.setText(mListHeaderViewTitleSelectDefault);
            } else {
                mListHeaderViewTitle.setText(mListHeaderViewTitleNoActivities);
            }
            mAdapter.setHeaderView(mListHeaderView);
        } else {
            mAdapter.setHeaderView(null);
        }

        if (mAdapter.getActivityCount() > maxActivityCount + 1) {
            mAdapter.setFooterView(mListFooterView);
        } else {
            mAdapter.setFooterView(null);
        }

        if (!mIsAttachedToWindow || mIsShowingAlertDialog) {
            AlertDialog alertDialog = getAlertDilalog();
            if (!alertDialog.isShowing()) {
                alertDialog.setCustomTitle(this);
                alertDialog.show();
                mIsShowingAlertDialog = true;
            }
        } else {
            ListPopupWindow popupWindow = getListPopupWindow();
            if (!popupWindow.isShowing()) {
                popupWindow.setContentWidth(mAdapter.measureContentWidth());
                popupWindow.show();
                mIsShowingPopuWindow = true;
            }
        }
    }

    /**
     * Dismisses the popup window with activities.
     *
     * @return True if dismissed, false if already dismissed.
     */
    public boolean dismissPopup() {
        if (!isShowingPopup()) {
            return false;
        }
        if (mIsShowingAlertDialog) {
            getAlertDilalog().dismiss();
        } else if (mIsShowingPopuWindow) {
            getListPopupWindow().dismiss();
        }
        return true;
    }

    /**
     * Gets whether the popup window with activities is shown.
     *
     * @return True if the popup is shown.
     */
    public boolean isShowingPopup() {
        if (mIsShowingAlertDialog) {
            return getAlertDilalog().isShowing();
        } else if (mIsShowingPopuWindow) {
            return getListPopupWindow().isShowing();
        }
        return false;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        ActivityChooserModel dataModel = mAdapter.getDataModel();
        if (dataModel != null) {
            dataModel.readHistoricalData();
        }
        mIsAttachedToWindow = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        ActivityChooserModel dataModel = mAdapter.getDataModel();
        if (dataModel != null) {
            dataModel.persistHistoricalData();
        }
        mIsAttachedToWindow = false;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mActivityChooserContent.measure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(mActivityChooserContent.getMeasuredWidth(),
                mActivityChooserContent.getMeasuredHeight());
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        mActivityChooserContent.layout(left, top, right, bottom);
        if (mIsShowingPopuWindow) {
            if (isShown()) {
                showPopupUnchecked(mAdapter.getMaxActivityCount());
            } else {
                dismissPopup();
            }
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        mActivityChooserContent.onDraw(canvas);
    }

    public ActivityChooserModel getDataModel() {
        return mAdapter.getDataModel();
    }

    /**
     * Sets a listener to receive a callback when the popup is dismissed.
     *
     * @param listener The listener to be notified.
     */
    public void setOnDismissListener(PopupWindow.OnDismissListener listener) {
        mOnDismissListener = listener;
    }

    /**
     * Sets the initial count of items shown in the activities popup
     * i.e. the items before the popup is expanded. This is an upper
     * bound since it is not guaranteed that such number of intent
     * handlers exist.
     *
     * @param itemCount The initial popup item count.
     */
    public void setInitialActivityCount(int itemCount) {
        mInitialActivityCount = itemCount;
    }

    /**
     * Gets the list popup window which is lazily initialized.
     *
     * @return The popup.
     */
    private ListPopupWindow getListPopupWindow() {
        if (mListPopupWindow == null) {
            mListPopupWindow = new ListPopupWindow(getContext());
            mListPopupWindow.setAdapter(mAdapter);
            mListPopupWindow.setAnchorView(ActivityChooserView.this);
            mListPopupWindow.setModal(true);
            mListPopupWindow.setOnItemClickListener(mCallbacks);
            mListPopupWindow.setOnDismissListener(mCallbacks);
        }
        return mListPopupWindow;
    }

    /**
     * Gets the alert dialog which is lazily initialized.
     *
     * @return The popup.
     */
    private AlertDialog getAlertDilalog() {
        if (mAlertDialog == null) {
            Builder builder = new Builder(getContext());
            builder.setAdapter(mAdapter, null);
            mAlertDialog = builder.create();
            mAlertDialog.getListView().setOnItemClickListener(mCallbacks);
            mAlertDialog.setOnDismissListener(mCallbacks);
        }
        return mAlertDialog;
    }

    /**
     * Updates the buttons state.
     */
    private void updateButtons() {
        final int activityCount = mAdapter.getActivityCount();
        if (activityCount > 0) {
            mDefaultActionButton.setVisibility(VISIBLE);
            if (mAdapter.getCount() > 0) {
                mExpandActivityOverflowButton.setEnabled(true);
            } else {
                mExpandActivityOverflowButton.setEnabled(false);
            }
            ResolveInfo activity = mAdapter.getDefaultActivity();
            PackageManager packageManager = mContext.getPackageManager();
            mDefaultActionButton.setBackgroundDrawable(activity.loadIcon(packageManager));
        } else {
            mDefaultActionButton.setVisibility(View.INVISIBLE);
            mExpandActivityOverflowButton.setEnabled(false);
        }
    }

    /**
     * Interface implementation to avoid publishing them in the APIs.
     */
    private class Callbacks implements AdapterView.OnItemClickListener,
            View.OnClickListener, View.OnLongClickListener, PopupWindow.OnDismissListener,
            DialogInterface.OnDismissListener {

        // AdapterView#OnItemClickListener
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            ActivityChooserViewAdapter adapter = (ActivityChooserViewAdapter) parent.getAdapter();
            final int itemViewType = adapter.getItemViewType(position);
            switch (itemViewType) {
                case ActivityChooserViewAdapter.ITEM_VIEW_TYPE_HEADER: {
                    /* do nothing */
                } break;
                case ActivityChooserViewAdapter.ITEM_VIEW_TYPE_FOOTER: {
                    showPopupUnchecked(ActivityChooserViewAdapter.MAX_ACTIVITY_COUNT_UNLIMITED);
                } break;
                case ActivityChooserViewAdapter.ITEM_VIEW_TYPE_ACTIVITY: {
                    dismissPopup();
                    if (mIsSelectingDefaultActivity) {
                        mAdapter.getDataModel().setDefaultActivity(position);
                    } else {
                        // The first item in the model is default action => adjust index
                        Intent launchIntent = mAdapter.getDataModel().chooseActivity(position + 1);
                        mContext.startActivity(launchIntent);
                    }
                } break;
                default:
                    throw new IllegalArgumentException();
            }
        }

        // View.OnClickListener
        public void onClick(View view) {
            if (view == mDefaultActionButton) {
                dismissPopup();
                ResolveInfo defaultActivity = mAdapter.getDefaultActivity();
                final int index = mAdapter.getDataModel().getActivityIndex(defaultActivity);
                Intent launchIntent = mAdapter.getDataModel().chooseActivity(index);
                mContext.startActivity(launchIntent);
            } else if (view == mExpandActivityOverflowButton) {
                mIsSelectingDefaultActivity = false;
                showPopupUnchecked(mInitialActivityCount);
            } else {
                throw new IllegalArgumentException();
            }
        }

        // OnLongClickListener#onLongClick
        @Override
        public boolean onLongClick(View view) {
            if (view == mDefaultActionButton) {
                if (mAdapter.getCount() > 0) {
                    mIsSelectingDefaultActivity = true;
                    showPopupUnchecked(mInitialActivityCount);
                }
            } else {
                throw new IllegalArgumentException();
            }
            return true;
        }

        // PopUpWindow.OnDismissListener#onDismiss
        public void onDismiss() {
            mIsShowingPopuWindow = false;
            notifyOnDismissListener();
        }

        // DialogInterface.OnDismissListener#onDismiss
        @Override
        public void onDismiss(DialogInterface dialog) {
            mIsShowingAlertDialog = false;
            AlertDialog alertDialog = (AlertDialog) dialog;
            alertDialog.setCustomTitle(null);
            notifyOnDismissListener();
        }

        private void notifyOnDismissListener() {
            if (mOnDismissListener != null) {
                mOnDismissListener.onDismiss();
            }
        }
    }

    /**
     * Adapter for backing the list of activities shown in the popup.
     */
    private class ActivityChooserViewAdapter extends BaseAdapter {

        public static final int MAX_ACTIVITY_COUNT_UNLIMITED = Integer.MAX_VALUE;

        public static final int MAX_ACTIVITY_COUNT_DEFAULT = 4;

        private static final int ITEM_VIEW_TYPE_HEADER = 0;

        private static final int ITEM_VIEW_TYPE_ACTIVITY = 1;

        private static final int ITEM_VIEW_TYPE_FOOTER = 2;

        private static final int ITEM_VIEW_TYPE_COUNT = 3;

        private final DataSetObserver mDataSetOberver = new DataSetObserver() {

            @Override
            public void onChanged() {
                super.onChanged();
                notifyDataSetChanged();
            }
            @Override
            public void onInvalidated() {
                super.onInvalidated();
                notifyDataSetInvalidated();
            }
        };

        private ActivityChooserModel mDataModel;

        private int mMaxActivityCount = MAX_ACTIVITY_COUNT_DEFAULT;

        private ResolveInfo mDefaultActivity;

        private View mHeaderView;

        private View mFooterView;

        public void setDataModel(ActivityChooserModel dataModel) {
            mDataModel = dataModel;
            mDataModel.registerObserver(mDataSetOberver);
            notifyDataSetChanged();
        }

        @Override
        public void notifyDataSetChanged() {
            if (mDataModel.getActivityCount() > 0) {
                mDefaultActivity = mDataModel.getDefaultActivity();
            } else {
                mDefaultActivity = null;
            }
            super.notifyDataSetChanged();
        }

        @Override
        public int getItemViewType(int position) {
            if (mHeaderView != null && position == 0) {
                return ITEM_VIEW_TYPE_HEADER;
            } else if (mFooterView != null && position == getCount() - 1) {
                return ITEM_VIEW_TYPE_FOOTER;
            } else {
                return ITEM_VIEW_TYPE_ACTIVITY;
            }
        }

        @Override
        public int getViewTypeCount() {
            return ITEM_VIEW_TYPE_COUNT;
        }

        public int getCount() {
            int count = 0;
            int activityCount = mDataModel.getActivityCount();
            if (activityCount > 0) {
                activityCount--;
            }
            count = Math.min(activityCount, mMaxActivityCount);
            if (mHeaderView != null) {
                count++;
            }
            if (mFooterView != null) {
                count++;
            }
            return count;
        }

        public Object getItem(int position) {
            final int itemViewType = getItemViewType(position);
            switch (itemViewType) {
                case ITEM_VIEW_TYPE_HEADER:
                    return mHeaderView;
                case ITEM_VIEW_TYPE_FOOTER:
                    return mFooterView;
                case ITEM_VIEW_TYPE_ACTIVITY:
                    int targetIndex = (mHeaderView == null) ? position : position - 1;
                    if (mDefaultActivity != null) {
                        targetIndex++;
                    }
                    return mDataModel.getActivity(targetIndex);
                default:
                    throw new IllegalArgumentException();
            }
        }

        public long getItemId(int position) {
            return position;
        }

        @Override
        public boolean isEnabled(int position) {
            final int itemViewType = getItemViewType(position);
            switch (itemViewType) {
                case ITEM_VIEW_TYPE_HEADER:
                    return false;
                case ITEM_VIEW_TYPE_FOOTER:
                case ITEM_VIEW_TYPE_ACTIVITY:
                    return true;
                default:
                    throw new IllegalArgumentException();
            }
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            final int itemViewType = getItemViewType(position);
            switch (itemViewType) {
                case ITEM_VIEW_TYPE_HEADER:
                    return mHeaderView;
                case ITEM_VIEW_TYPE_FOOTER:
                    return mFooterView;
                case ITEM_VIEW_TYPE_ACTIVITY:
                    if (convertView == null || convertView.getId() != R.id.list_item) {
                        convertView = LayoutInflater.from(getContext()).inflate(
                                R.layout.activity_chooser_view_list_item, parent, false);
                    }
                    PackageManager packageManager = mContext.getPackageManager();
                    // Set the icon
                    ImageView iconView = (ImageView) convertView.findViewById(R.id.icon);
                    ResolveInfo activity = (ResolveInfo) getItem(position);
                    iconView.setBackgroundDrawable(activity.loadIcon(packageManager));
                    // Set the title.
                    TextView titleView = (TextView) convertView.findViewById(R.id.title);
                    titleView.setText(activity.loadLabel(packageManager));
                    return convertView;
                default:
                    throw new IllegalArgumentException();
            }
        }

        public int measureContentWidth() {
            // The user may have specified some of the target not to be show but we
            // want to measure all of them since after expansion they should fit.
            final int oldMaxActivityCount = mMaxActivityCount;
            mMaxActivityCount = MAX_ACTIVITY_COUNT_UNLIMITED;

            int contentWidth = 0;
            View itemView = null;

            final int widthMeasureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
            final int heightMeasureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
            final int count = getCount();

            for (int i = 0; i < count; i++) {
                itemView = getView(i, itemView, null);
                itemView.measure(widthMeasureSpec, heightMeasureSpec);
                contentWidth = Math.max(contentWidth, itemView.getMeasuredWidth());
            }

            mMaxActivityCount = oldMaxActivityCount;

            return contentWidth;
        }

        public void setMaxActivityCount(int maxActivityCount) {
            if (mMaxActivityCount != maxActivityCount) {
                mMaxActivityCount = maxActivityCount;
                notifyDataSetChanged();
            }
        }

        public ResolveInfo getDefaultActivity() {
            return mDefaultActivity;
        }

        public void setHeaderView(View headerView) {
            if (mHeaderView != headerView) {
                mHeaderView = headerView;
                notifyDataSetChanged();
            }
        }

        public void setFooterView(View footerView) {
            if (mFooterView != footerView) {
                mFooterView = footerView;
                notifyDataSetChanged();
            }
        }

        public int getActivityCount() {
            return mDataModel.getActivityCount();
        }

        public int getMaxActivityCount() {
            return mMaxActivityCount;
        }

        public ActivityChooserModel getDataModel() {
            return mDataModel;
        }
    }
}