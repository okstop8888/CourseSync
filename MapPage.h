#pragma once
#include "resource.h"
#include "CourseData.h"
#include "CourseConverter.h"

class MapPage : public CPropertyPage
{
    DECLARE_DYNAMIC(MapPage)
public:
    MapPage();

    // 从外部刷新来源列名（由 SrcPage 切换表后调用）
    void refreshSourceColumns(const vector<string> &cols, const map<string,string> &sampleRow);

    FieldMap getFieldMap() const;
    void     applyFieldMap(const FieldMap &m);

    enum { IDD = IDD_PAGE_MAP };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange *pDX);

    afx_msg void OnBtnResetMap();
    afx_msg void OnBtnPreviewMap();
    afx_msg void OnBtnAddMap();
    afx_msg void OnBtnDelMap();
    afx_msg void OnDblclkListMap(NMHDR *pNMHDR, LRESULT *pResult);
    DECLARE_MESSAGE_MAP()

private:
    CListCtrl m_listMap;

    void fillList(const FieldMap &m, const vector<string> &srcCols);
    void editSelectedRow();

    vector<string> m_srcCols;
    map<string,string> m_sampleRow;

    // ct_course 可映射目标字段列表
    static vector<string> ctFields();
};
